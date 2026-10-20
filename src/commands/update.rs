use anyhow::{Context, Result};
use std::env;
use std::fs;
use std::path::Path;
use std::process::{Command, Stdio};

use crate::registry::get_all_records;
use crate::utils::{confirm, root_required, try_clone_with_fallback};

pub fn update(yes: bool, verbose: bool) -> Result<()> {
    root_required()?;

    let records = get_all_records()?;
    let updatable: Vec<_> = records
        .into_iter()
        .filter(|r| r.git_url.is_some())
        .collect();

    if updatable.is_empty() {
        println!("No applications with git repositories found for update.");
        return Ok(());
    }

    println!("Found {} application(s) with git repositories:", updatable.len());
    for record in &updatable {
        println!("  - {} ({})", record.name, record.git_url.as_ref().unwrap());
    }

    if !yes && !confirm("Update these applications? [y/N] ")? {
        println!("Cancelled.");
        return Ok(());
    }

    let mut failed = Vec::new();
    let mut updated = Vec::new();

    let sudo_user = std::env::var("SUDO_USER").ok();

    for record in updatable {
        let git_url = record.git_url.as_ref().unwrap();
        let name = &record.name;

        println!("\nUpdating {}...", name);

        // Create a temporary directory for cloning
        // Use /tmp when running under sudo to avoid permission issues
        let temp_dir = if sudo_user.is_some() {
            Path::new("/tmp").join(format!("makekeeper-update-{}-{}", name, std::process::id()))
        } else {
            env::temp_dir().join(format!("makekeeper-update-{}-{}", name, std::process::id()))
        };

        // Create temp directory as the user if running under sudo
        if let Some(ref user) = sudo_user {
            Command::new("sudo")
                .arg("-u")
                .arg(user)
                .arg("mkdir")
                .arg("-p")
                .arg(&temp_dir)
                .status()
                .context("failed to create temporary directory")?;
        } else {
            fs::create_dir_all(&temp_dir)
                .context("failed to create temporary directory for update")?;
        }

        // Clone the repository with SSH->HTTPS fallback
        // Drop privileges to the original user for git clone to access SSH keys
        if let Some(ref user) = sudo_user {
            // Running under sudo - drop to original user for git clone
            let status = Command::new("sudo")
                .arg("-u")
                .arg(user)
                .arg("git")
                .arg("clone")
                .arg("--quiet")
                .arg(git_url)
                .arg(&temp_dir)
                .stdout(Stdio::null())
                .stderr(Stdio::null())
                .status();

            match status {
                Ok(s) if s.success() => {},
                Ok(_) => {
                    let _ = fs::remove_dir_all(&temp_dir);
                    eprintln!("Failed to clone repository for {}", name);
                    failed.push(name.clone());
                    continue;
                }
                Err(e) => {
                    let _ = fs::remove_dir_all(&temp_dir);
                    eprintln!("Failed to clone repository for {}: {}", name, e);
                    failed.push(name.clone());
                    continue;
                }
            }
        } else {
            // Not running under sudo - use fallback logic
            if let Err(e) = try_clone_with_fallback(&temp_dir, git_url) {
                let _ = fs::remove_dir_all(&temp_dir);
                eprintln!("Failed to clone repository for {}: {}", name, e);
                failed.push(name.clone());
                continue;
            }
        }

        // Reinstall using the install command (URL as-is)
        let result = crate::commands::install::install(
            temp_dir.as_path(),
            Some(name.clone()),
            true,  // yes
            true,  // override
            verbose,
            Some(git_url.clone()),
        );

        // Clean up the cloned directory
        let _ = fs::remove_dir_all(&temp_dir);

        match result {
            Ok(_) => {
                println!("Updated {}", name);
                updated.push(name.clone());
            }
            Err(e) => {
                eprintln!("Failed to update {}: {}", name, e);
                failed.push(name.clone());
            }
        }
    }

    println!("\nUpdate summary:");
    println!("  Updated: {}", updated.len());
    println!("  Failed: {}", failed.len());

    if !failed.is_empty() {
        eprintln!("Failed applications:");
        for name in failed {
            eprintln!("  - {}", name);
        }
    }

    Ok(())
}
