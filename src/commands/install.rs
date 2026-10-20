use anyhow::{Context, Result, bail};
use std::fs;
use std::os::unix::fs::{PermissionsExt, symlink};
use std::path::Path;
use std::process::{Command, Stdio};
use walkdir::WalkDir;

use crate::config::{APP_ROOT, BIN_DIR, MAN_DIR};
use crate::models::{Record, InstalledFile, ManagedLink};
use crate::registry::{record_path, rollback_files, rollback_links, save_record};
use crate::utils::{confirm, hash_file, now, project_dir, root_required, valid_name};
use crate::commands::uninstall;

pub fn install(project_arg: &Path, name_arg: Option<String>, yes: bool, override_: bool, verbose: bool, git_url_arg: Option<String>) -> Result<()> {
    root_required()?;
    let project = project_dir(project_arg)?;
    let default_name = project
        .file_name()
        .unwrap_or_default()
        .to_string_lossy()
        .to_string();
    let custom_name_provided = name_arg.is_some();
    let name = match name_arg {
        Some(n) => n,
        None => default_name,
    };

    if !valid_name(&name) {
        bail!("invalid application name");
    }

    let record_file = record_path(&name);
    if record_file.exists() {
        if !override_ {
            bail!("{name} is already registered");
        }
    }

    let prefix = Path::new(APP_ROOT).join(&name);
    if fs::symlink_metadata(&prefix).is_ok() && !override_ {
        bail!("installation prefix already exists: {}", prefix.display());
    }

    if verbose {
        println!("Source:  {}", project.display());
        println!("Name:    {name}");
        println!("Prefix:  {}", prefix.display());
        println!("Links:   executable files in {APP_ROOT} will be linked into {BIN_DIR}");
        println!("\nMakefiles can execute arbitrary code. Only install projects you trust.");
    }

    // Early check: if the app name already exists in /usr/local/bin, warn but continue.
    let potential_link = Path::new(BIN_DIR).join(&name);
    if fs::symlink_metadata(&potential_link).is_ok() {
        if verbose {
            eprintln!("warning: {} will be overwritten", potential_link.display());
        }
    }

    if !yes && !confirm("Build and install this project? [y/N] ")? {
        println!("Cancelled.");
        return Ok(());
    }

    // Now uninstall if override is set and installation exists
    if override_ && record_file.exists() {
        if verbose {
            eprintln!("Overriding existing installation of {name}...");
        }
        uninstall::uninstall(&name, true, false)?;
    }

    // Build the default Make target.
    let make_status = if verbose {
        Command::new("make").current_dir(&project).status()
    } else {
        Command::new("make")
            .current_dir(&project)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
    };
    let status = make_status?;
    if !status.success() {
        bail!("make failed; installation stopped");
    }

    // Stage the install before copying anything into the real prefix.
    let stage = std::env::temp_dir().join(format!("makekeeper-{}-{}", std::process::id(), now()));
    fs::create_dir(&stage)?;

    let result = stage_install(&project, &prefix, &stage, verbose);
    if let Err(e) = result {
        let _ = fs::remove_dir_all(&stage);
        return Err(e);
    }

    let staged_root = stage.join(prefix.strip_prefix("/")?);
    if !staged_root.is_dir() {
        let _ = fs::remove_dir_all(&stage);
        bail!(
            "no staged files appeared under {}. The project may not honor PREFIX and DESTDIR",
            prefix.display()
        );
    }

    let mut staged_files = Vec::new();
    // Walk the entire stage directory to catch files installed outside the prefix
    for entry in WalkDir::new(&stage).follow_links(false) {
        let entry = entry?;
        if entry.file_type().is_dir() {
            continue;
        }
        let relative = entry.path().strip_prefix(&stage)?;
        staged_files.push((entry.path().to_path_buf(), Path::new("/").join(relative)));
    }

    if staged_files.is_empty() {
        let _ = fs::remove_dir_all(&stage);
        bail!("staged installation contains no files");
    }

    // Warn about existing files that will be overwritten.
    if verbose {
        for (_, destination) in &staged_files {
            if fs::symlink_metadata(destination).is_ok() {
                eprintln!("warning: {} will be overwritten", destination.display());
            }
        }
    }

    // Warn about symlink conflicts before asking for confirmation.
    for (source, _) in &staged_files {
        let is_bin_file = source
            .parent()
            .and_then(Path::file_name)
            .is_some_and(|part| part == "bin");

        let metadata = fs::symlink_metadata(source)?;
        let executable = metadata.is_file() && metadata.permissions().mode() & 0o111 != 0;

        if !is_bin_file || !executable {
            continue;
        }

        let Some(filename) = source.file_name() else {
            continue;
        };
        // Use custom name if provided, otherwise use original filename
        let link_name = if custom_name_provided {
            name.clone()
        } else {
            filename.to_string_lossy().into_owned()
        };
        let link = Path::new(BIN_DIR).join(link_name);

        if fs::symlink_metadata(&link).is_ok() {
            eprintln!("warning: {} will be overwritten", link.display());
        }
    }

    // Check for man page conflicts.
    for (source, _) in &staged_files {
        let path_str = source.to_string_lossy();

        // Check if path contains share/man/manN
        if !path_str.contains("share/man/man") {
            continue;
        }

        // Extract the man section (man1, man2, etc.)
        let man_section = path_str
            .split("share/man/man")
            .nth(1)
            .and_then(|s| s.chars().next());

        if man_section.is_none() {
            continue;
        }

        let Some(filename) = source.file_name() else {
            continue;
        };

        let man_section_str = format!("man{}", man_section.unwrap());
        // Use custom name if provided, otherwise use original filename
        let link_name = if custom_name_provided {
            let ext = Path::new(filename).extension().and_then(|s| s.to_str()).unwrap_or("1");
            format!("{}.{}", name, ext)
        } else {
            filename.to_string_lossy().into_owned()
        };
        let link = Path::new(MAN_DIR).join(&man_section_str).join(link_name);

        if fs::symlink_metadata(&link).is_ok() {
            eprintln!("warning: {} will be overwritten", link.display());
        }
    }

    // Detect if this is a git repository and extract the remote URL
    // Use the provided git_url if available, otherwise detect it
    // Store the URL as-is (don't convert)
    let git_url = git_url_arg.or_else(|| detect_git_url(&project));

    // Get the current commit hash if this is a git repository
    let commit_hash = get_git_commit_hash(&project);

    let mut record = Record {
        name: name.clone(),
        source: project.to_string_lossy().into_owned(),
        prefix: prefix.to_string_lossy().into_owned(),
        installed_at: now(),
        files: Vec::new(),
        links: Vec::new(),
        git_url,
        last_commit_hash: commit_hash.clone(),
    };

    // Copy staged entries, recording hashes and symlink targets.
    let copy_result: Result<()> = (|| {
        for (source, destination) in &staged_files {
            let metadata = fs::symlink_metadata(source)?;
            fs::create_dir_all(destination.parent().context("missing parent directory")?)?;

            // Remove existing file/symlink if it exists
            if fs::symlink_metadata(destination).is_ok() {
                fs::remove_file(destination)?;
            }

            if metadata.file_type().is_symlink() {
                let target = fs::read_link(source)?;
                symlink(&target, destination)?;
                if verbose {
                    println!("Created symlink: {}", destination.display());
                }
                record.files.push(InstalledFile {
                    path: destination.to_string_lossy().into_owned(),
                    sha256: None,
                    link_target: Some(target.to_string_lossy().into_owned()),
                });
            } else if metadata.is_file() {
                fs::copy(source, destination)?;
                fs::set_permissions(
                    destination,
                    fs::Permissions::from_mode(metadata.permissions().mode() & 0o777),
                )?;
                if verbose {
                    println!("Installed: {}", destination.display());
                }
                record.files.push(InstalledFile {
                    path: destination.to_string_lossy().into_owned(),
                    sha256: Some(hash_file(destination)?),
                    link_target: None,
                });
            } else {
                bail!("unsupported staged file type: {}", source.display());
            }
        }
        Ok(())
    })();

    if let Err(e) = copy_result {
        rollback_files(&record.files);
        let _ = fs::remove_dir_all(&prefix);
        let _ = fs::remove_dir_all(&stage);
        return Err(e).context("copy failed; attempted rollback");
    }

    // Only executable files directly in a bin directory get PATH links.
    for file in &record.files {
        let target = Path::new(&file.path);
        let is_bin_file = target
            .parent()
            .and_then(Path::file_name)
            .is_some_and(|part| part == "bin");

        let executable = fs::symlink_metadata(target)
            .map(|m| m.is_file() && m.permissions().mode() & 0o111 != 0)
            .unwrap_or(false);

        if !is_bin_file || !executable {
            continue;
        }

        let Some(filename) = target.file_name() else {
            continue;
        };
        // Use custom name if provided, otherwise use original filename
        let link_name = if custom_name_provided {
            name.clone()
        } else {
            filename.to_string_lossy().into_owned()
        };
        let link = Path::new(BIN_DIR).join(link_name);

        // Remove existing symlink if it exists
        if fs::symlink_metadata(&link).is_ok() {
            fs::remove_file(&link)?;
        }

        symlink(target, &link).with_context(|| format!("could not create {}", link.display()))?;

        if verbose {
            println!("Created symlink: {} -> {}", link.display(), target.display());
        }

        record.links.push(ManagedLink {
            path: link.to_string_lossy().into_owned(),
            target: target.to_string_lossy().into_owned(),
        });
    }

    // Man pages in share/man/man*/ directories get symlinked to /usr/share/man.
    for file in &record.files {
        let target = Path::new(&file.path);
        let path_str = target.to_string_lossy();

        // Check if path contains share/man/manN
        if !path_str.contains("share/man/man") {
            continue;
        }

        // Extract the man section (man1, man2, etc.)
        let man_section = path_str
            .split("share/man/man")
            .nth(1)
            .and_then(|s| s.chars().next());

        if man_section.is_none() {
            continue;
        }

        let Some(filename) = target.file_name() else {
            continue;
        };

        // Build the link path: /usr/share/man/manN/filename
        let man_section_str = format!("man{}", man_section.unwrap());
        // Use custom name if provided, otherwise use original filename
        let link_name = if custom_name_provided {
            // Replace the base name with the custom name, preserving the extension
            let ext = Path::new(filename).extension().and_then(|s| s.to_str()).unwrap_or("1");
            format!("{}.{}", name, ext)
        } else {
            filename.to_string_lossy().into_owned()
        };
        let link = Path::new(MAN_DIR).join(&man_section_str).join(link_name);

        // Remove existing symlink if it exists
        if fs::symlink_metadata(&link).is_ok() {
            fs::remove_file(&link)?;
        }

        // Create parent directory if needed
        if let Some(parent) = link.parent() {
            fs::create_dir_all(parent)?;
        }

        symlink(target, &link).with_context(|| format!("could not create {}", link.display()))?;

        if verbose {
            println!("Created symlink: {} -> {}", link.display(), target.display());
        }

        record.links.push(ManagedLink {
            path: link.to_string_lossy().into_owned(),
            target: target.to_string_lossy().into_owned(),
        });
    }

    if let Err(e) = save_record(&record) {
        rollback_links(&record.links);
        rollback_files(&record.files);
        let _ = fs::remove_dir_all(&prefix);
        let _ = fs::remove_dir_all(&stage);
        return Err(e).context("could not save registry; attempted rollback");
    }

    let _ = fs::remove_dir_all(&stage);
    if verbose {
        println!(
            "Installed {name}: {} files, {} symlinks",
            record.files.len(),
            record.links.len()
        );
    }
    Ok(())
}

fn stage_install(project: &Path, prefix: &Path, stage: &Path, verbose: bool) -> Result<()> {
    let prefix_arg = format!("PREFIX={}", prefix.display());
    let destdir_arg = format!("DESTDIR={}", stage.display());

    let status = if verbose {
        Command::new("make")
            .current_dir(project)
            .arg("install")
            .arg(&prefix_arg)
            .arg(&destdir_arg)
            .status()
    } else {
        Command::new("make")
            .current_dir(project)
            .arg("install")
            .arg(&prefix_arg)
            .arg(&destdir_arg)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
    }
    .context("could not run make install")?;

    if !status.success() {
        bail!("make install failed");
    }
    Ok(())
}

fn detect_git_url(project: &Path) -> Option<String> {
    let git_dir = project.join(".git");
    if !git_dir.exists() {
        return None;
    }

    // Try to get the remote URL from git config
    let output = Command::new("git")
        .current_dir(project)
        .args(["config", "--get", "remote.origin.url"])
        .output()
        .ok()?;

    if output.status.success() {
        let url = String::from_utf8(output.stdout).ok()?;
        let url = url.trim();
        if !url.is_empty() {
            return Some(url.to_string());
        }
    }

    None
}

pub fn get_git_commit_hash(project: &Path) -> Option<String> {
    let git_dir = project.join(".git");
    if !git_dir.exists() {
        return None;
    }

    // Try to get the current commit hash
    let output = Command::new("git")
        .current_dir(project)
        .args(["rev-parse", "HEAD"])
        .output()
        .ok()?;

    if output.status.success() {
        let hash_str = String::from_utf8(output.stdout).ok()?;
        let hash_str = hash_str.trim();
        if !hash_str.is_empty() {
            return Some(hash_str.to_string());
        }
    }

    None
}
