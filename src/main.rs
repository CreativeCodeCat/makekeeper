use anyhow::{Context, Result, bail};
use clap::{Parser, Subcommand};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::BTreeSet,
    fs::{self, File},
    io::{self, Read, Write},
    os::unix::fs::{PermissionsExt, symlink},
    path::{Path, PathBuf},
    process::Command,
    time::{SystemTime, UNIX_EPOCH},
};
use walkdir::WalkDir;

const REGISTRY: &str = "/var/lib/makekeeper";
const APP_ROOT: &str = "/opt/makekeeper/apps";
const BIN_DIR: &str = "/usr/local/bin";
const MAN_DIR: &str = "/usr/share/man";

#[derive(Parser)]
#[command(
    version,
    about = "A safer installer for Makefile-based Linux applications"
)]
struct Cli {
    #[command(subcommand)]
    command: Action,
}

#[derive(Subcommand)]
enum Action {
    /// Inspect a project's Makefile and preview installation commands.
    Inspect { project: PathBuf },
    /// Build and install a project.
    Install {
        project: PathBuf,
        #[arg(long)]
        name: Option<String>,
        #[arg(long)]
        yes: bool,
    },
    /// List registered applications.
    List,
    /// Display an installation record.
    Info { name: String },
    /// Remove tracked files and managed executable symlinks.
    Uninstall {
        name: String,
        #[arg(long)]
        yes: bool,
    },
}

#[derive(Serialize, Deserialize)]
struct Record {
    name: String,
    source: String,
    prefix: String,
    installed_at: u64,
    files: Vec<InstalledFile>,
    links: Vec<ManagedLink>,
}

#[derive(Serialize, Deserialize)]
struct InstalledFile {
    path: String,
    sha256: Option<String>,
    link_target: Option<String>,
}

#[derive(Serialize, Deserialize)]
struct ManagedLink {
    path: String,
    target: String,
}

fn main() {
    if let Err(e) = run() {
        eprintln!("makekeeper: {e:#}");
        std::process::exit(1);
    }
}

fn run() -> Result<()> {
    match Cli::parse().command {
        Action::Inspect { project } => inspect(&project),
        Action::Install { project, name, yes } => install(&project, name, yes),
        Action::List => list(),
        Action::Info { name } => info(&name),
        Action::Uninstall { name, yes } => uninstall(&name, yes),
    }
}

fn root_required() -> Result<()> {
    let out = Command::new("id").arg("-u").output()?;
    if !String::from_utf8_lossy(&out.stdout).trim().eq("0") {
        bail!("system-wide operation requires root; run this command with sudo");
    }
    Ok(())
}

fn valid_name(s: &str) -> bool {
    !s.is_empty()
        && s.len() <= 80
        && s != "."
        && s != ".."
        && s.chars()
            .all(|c| c.is_ascii_alphanumeric() || "._-".contains(c))
}

fn project_dir(path: &Path) -> Result<PathBuf> {
    let p = fs::canonicalize(path)?;
    if !p.is_dir() {
        bail!("project path is not a directory");
    }
    if !["Makefile", "makefile", "GNUmakefile"]
        .iter()
        .any(|f| p.join(f).is_file())
    {
        bail!("no Makefile found in {}", p.display());
    }
    Ok(p)
}

fn inspect(project: &Path) -> Result<()> {
    let project = project_dir(project)?;
    println!("Project: {}", project.display());

    // -qp prints Make's database. It may return a nonzero status
    // because -q reports whether targets need rebuilding.
    let output = Command::new("make")
        .current_dir(&project)
        .args(["-qp"])
        .output()
        .context("could not run GNU Make")?;

    let mut targets = BTreeSet::new();
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        if line.starts_with('#') || line.starts_with('\t') || line.starts_with(' ') {
            continue;
        }
        if let Some((lhs, rhs)) = line.split_once(':') {
            if !lhs.is_empty() && !lhs.contains(['=', '$', '/', '%', ' ']) && !rhs.starts_with('=')
            {
                targets.insert(lhs.to_string());
            }
        }
    }

    println!("\nPossible targets:");
    for target in targets {
        println!("  {target}");
    }

    println!("\nPreview: make -n install");
    println!("Warning: a Makefile can run arbitrary commands; this is not a sandbox.");

    let preview = Command::new("make")
        .current_dir(&project)
        .args([
            "-n",
            "install",
            "PREFIX=/opt/makekeeper/apps/EXAMPLE",
            "DESTDIR=/tmp/makekeeper-stage",
        ])
        .output()?;

    io::stdout().write_all(&preview.stdout)?;
    io::stderr().write_all(&preview.stderr)?;
    println!("\nReview the Makefile before installing it.");
    Ok(())
}

fn install(project_arg: &Path, name: Option<String>, yes: bool) -> Result<()> {
    root_required()?;
    let project = project_dir(project_arg)?;
    let name = name.unwrap_or_else(|| {
        project
            .file_name()
            .unwrap_or_default()
            .to_string_lossy()
            .to_string()
    });

    if !valid_name(&name) {
        bail!("invalid application name");
    }

    let record_file = record_path(&name);
    if record_file.exists() {
        bail!("{name} is already registered");
    }

    let prefix = Path::new(APP_ROOT).join(&name);
    if fs::symlink_metadata(&prefix).is_ok() {
        bail!("installation prefix already exists: {}", prefix.display());
    }

    println!("Source:  {}", project.display());
    println!("Name:    {name}");
    println!("Prefix:  {}", prefix.display());
    println!("Links:   executable files in {APP_ROOT} will be linked into {BIN_DIR}");
    println!("\nMakefiles can execute arbitrary code. Only install projects you trust.");

    // Early check: if the app name already exists in /usr/local/bin, warn but continue.
    let potential_link = Path::new(BIN_DIR).join(&name);
    if fs::symlink_metadata(&potential_link).is_ok() {
        eprintln!("warning: {} will be overwritten", potential_link.display());
    }

    if !yes && !confirm("Build and install this project? [y/N] ")? {
        println!("Cancelled.");
        return Ok(());
    }

    // Build the default Make target.
    let status = Command::new("make").current_dir(&project).status()?;
    if !status.success() {
        bail!("make failed; installation stopped");
    }

    // Stage the install before copying anything into the real prefix.
    let stage = std::env::temp_dir().join(format!("makekeeper-{}-{}", std::process::id(), now()));
    fs::create_dir(&stage)?;

    let result = stage_install(&project, &prefix, &stage);
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
    for (_, destination) in &staged_files {
        if fs::symlink_metadata(destination).is_ok() {
            eprintln!("warning: {} will be overwritten", destination.display());
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
        let link = Path::new(BIN_DIR).join(filename);

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
        let link = Path::new(MAN_DIR).join(&man_section_str).join(filename);

        if fs::symlink_metadata(&link).is_ok() {
            eprintln!("warning: {} will be overwritten", link.display());
        }
    }

    let mut record = Record {
        name: name.clone(),
        source: project.to_string_lossy().into_owned(),
        prefix: prefix.to_string_lossy().into_owned(),
        installed_at: now(),
        files: Vec::new(),
        links: Vec::new(),
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
        let link = Path::new(BIN_DIR).join(filename);

        // Remove existing symlink if it exists
        if fs::symlink_metadata(&link).is_ok() {
            fs::remove_file(&link)?;
        }

        symlink(target, &link).with_context(|| format!("could not create {}", link.display()))?;

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
        let link = Path::new(MAN_DIR).join(&man_section_str).join(filename);

        // Remove existing symlink if it exists
        if fs::symlink_metadata(&link).is_ok() {
            fs::remove_file(&link)?;
        }

        // Create parent directory if needed
        if let Some(parent) = link.parent() {
            fs::create_dir_all(parent)?;
        }

        symlink(target, &link).with_context(|| format!("could not create {}", link.display()))?;

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
    println!(
        "Installed {name}: {} files, {} symlinks",
        record.files.len(),
        record.links.len()
    );
    Ok(())
}

fn stage_install(project: &Path, prefix: &Path, stage: &Path) -> Result<()> {
    let prefix_arg = format!("PREFIX={}", prefix.display());
    let destdir_arg = format!("DESTDIR={}", stage.display());

    let status = Command::new("make")
        .current_dir(project)
        .arg("install")
        .arg(&prefix_arg)
        .arg(&destdir_arg)
        .status()
        .context("could not run make install")?;

    if !status.success() {
        bail!("make install failed");
    }
    Ok(())
}

fn list() -> Result<()> {
    root_required()?;
    if !Path::new(REGISTRY).is_dir() {
        println!("No registered applications.");
        return Ok(());
    }

    let mut found = false;
    for entry in fs::read_dir(REGISTRY)? {
        let path = entry?.path();
        if path.extension().is_none_or(|e| e != "json") {
            continue;
        }
        let record: Record = serde_json::from_slice(&fs::read(path)?)?;
        println!(
            "{:<24} {} files, {} links",
            record.name,
            record.files.len(),
            record.links.len()
        );
        found = true;
    }

    if !found {
        println!("No registered applications.");
    }
    Ok(())
}

fn info(name: &str) -> Result<()> {
    root_required()?;
    let record = load_record(name)?;
    println!("Name:   {}", record.name);
    println!("Source: {}", record.source);
    println!("Prefix: {}", record.prefix);
    println!("Files:");
    for file in &record.files {
        println!("  {}", file.path);
    }
    println!("Symlinks:");
    for link in &record.links {
        println!("  {} -> {}", link.path, link.target);
    }
    Ok(())
}

fn uninstall(name: &str, yes: bool) -> Result<()> {
    root_required()?;
    let record = load_record(name)?;

    println!("Application: {name}");
    println!("Tracked files: {}", record.files.len());
    println!("Symlinks: {}", record.links.len());

    if !yes && !confirm("Remove unchanged tracked files? [y/N] ")? {
        println!("Cancelled.");
        return Ok(());
    }

    let mut skipped = false;

    for link in record.links.iter().rev() {
        let path = Path::new(&link.path);
        match fs::read_link(path) {
            Ok(target) if target.to_string_lossy() == link.target => {
                fs::remove_file(path)?;
                println!("Removed link {}", path.display());
            }
            Ok(_) => {
                eprintln!("Keeping modified symlink {}", path.display());
                skipped = true;
            }
            Err(e) if e.kind() == io::ErrorKind::NotFound => {}
            Err(e) => return Err(e.into()),
        }
    }

    for file in record.files.iter().rev() {
        let path = Path::new(&file.path);
        let meta = match fs::symlink_metadata(path) {
            Ok(m) => m,
            Err(e) if e.kind() == io::ErrorKind::NotFound => continue,
            Err(e) => return Err(e.into()),
        };

        let unchanged = if meta.file_type().is_symlink() {
            file.link_target
                .as_ref()
                .and_then(|expected| {
                    fs::read_link(path)
                        .ok()
                        .map(|actual| actual.to_string_lossy() == *expected)
                })
                .unwrap_or(false)
        } else if meta.is_file() {
            file.sha256
                .as_ref()
                .and_then(|expected| hash_file(path).ok().map(|actual| actual == *expected))
                .unwrap_or(false)
        } else {
            false
        };

        if unchanged {
            fs::remove_file(path)?;
            println!("Removed {}", path.display());
        } else {
            eprintln!("Keeping changed or unexpected file {}", path.display());
            skipped = true;
        }
    }

    // Remove only empty directories inside this application's prefix.
    let prefix = Path::new(&record.prefix);
    if prefix.is_dir() {
        for entry in WalkDir::new(prefix)
            .contents_first(true)
            .into_iter()
            .filter_map(Result::ok)
        {
            if entry.file_type().is_dir() {
                let _ = fs::remove_dir(entry.path());
            }
        }
        let _ = fs::remove_dir(prefix);
    }

    if skipped {
        println!("Some items were retained. Registry kept for manual review.");
    } else {
        fs::remove_file(record_path(name))?;
        println!("Uninstalled {name}.");
    }
    Ok(())
}

fn load_record(name: &str) -> Result<Record> {
    if !valid_name(name) {
        bail!("invalid application name");
    }

    let data =
        fs::read(record_path(name)).with_context(|| format!("no registry record for {name}"))?;
    let record: Record = serde_json::from_slice(&data)?;

    let expected_prefix = Path::new(APP_ROOT).join(name);
    if record.name != name || Path::new(&record.prefix) != expected_prefix {
        bail!("invalid registry record; refusing to operate");
    }

    for file in &record.files {
        if !Path::new(&file.path).starts_with(&expected_prefix) {
            bail!("unsafe file path in registry");
        }
    }

    for link in &record.links {
        let is_safe_path = Path::new(&link.path).starts_with(BIN_DIR)
            || Path::new(&link.path).starts_with(MAN_DIR);
        // Man page symlinks point to files under the prefix, executable symlinks also point to prefix
        if !is_safe_path || !Path::new(&link.target).starts_with(&expected_prefix) {
            bail!("unsafe symlink record");
        }
    }

    Ok(record)
}

fn save_record(record: &Record) -> Result<()> {
    fs::create_dir_all(REGISTRY)?;
    let path = record_path(&record.name);
    let temp = path.with_extension(format!("tmp-{}", std::process::id()));
    fs::write(&temp, serde_json::to_vec_pretty(record)?)?;
    fs::rename(temp, path)?;
    Ok(())
}

fn record_path(name: &str) -> PathBuf {
    Path::new(REGISTRY).join(format!("{name}.json"))
}

fn hash_file(path: &Path) -> Result<String> {
    let mut file = File::open(path)?;
    let mut hasher = Sha256::new();
    let mut buffer = [0u8; 16384];

    loop {
        let count = file.read(&mut buffer)?;
        if count == 0 {
            break;
        }
        hasher.update(&buffer[..count]);
    }

    Ok(format!("{:x}", hasher.finalize()))
}

fn rollback_files(files: &[InstalledFile]) {
    for file in files.iter().rev() {
        let path = Path::new(&file.path);
        let remove = if let Some(expected) = &file.sha256 {
            hash_file(path)
                .map(|actual| actual == *expected)
                .unwrap_or(false)
        } else if let Some(expected) = &file.link_target {
            fs::read_link(path)
                .map(|actual| actual.to_string_lossy() == *expected)
                .unwrap_or(false)
        } else {
            false
        };

        if remove {
            let _ = fs::remove_file(path);
        }
    }
}

fn rollback_links(links: &[ManagedLink]) {
    for link in links.iter().rev() {
        let path = Path::new(&link.path);
        if fs::read_link(path)
            .map(|target| target.to_string_lossy() == link.target)
            .unwrap_or(false)
        {
            let _ = fs::remove_file(path);
        }
    }
}

fn confirm(prompt: &str) -> Result<bool> {
    print!("{prompt}");
    io::stdout().flush()?;
    let mut answer = String::new();
    io::stdin().read_line(&mut answer)?;
    Ok(matches!(
        answer.trim().to_ascii_lowercase().as_str(),
        "y" | "yes"
    ))
}

fn now() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
