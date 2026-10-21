use anyhow::{Context, Result};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

use crate::registry::get_all_records;
use crate::utils::{confirm, root_required, try_clone_with_fallback};

fn get_cache_dir() -> PathBuf {
    Path::new("/tmp").join("makekeeper-cache")
}

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

    // Create cache directory
    let cache_dir = get_cache_dir();
    fs::create_dir_all(&cache_dir)
        .context("failed to create cache directory")?;

    for record in updatable {
        let git_url = record.git_url.as_ref().unwrap();
        let name = &record.name;

        println!("Updating {}...", name);

        // Create a cache directory for this repo
        // Use a sanitized version of the git URL as the cache name
        let cache_name = git_url
            .replace("://", "_")
            .replace("/", "_")
            .replace(":", "_")
            .replace("@", "_");
        let repo_cache_dir = cache_dir.join(&cache_name);

        // Clone or fetch the repository
        if repo_cache_dir.exists() {
            // Repository already cached, do a fetch
            let status = Command::new("git")
                .arg("fetch")
                .arg("--quiet")
                .current_dir(&repo_cache_dir)
                .stdout(Stdio::null())
                .stderr(Stdio::null())
                .status();

            match status {
                Ok(s) if s.success() => {},
                Ok(_) => {
                    eprintln!("Failed to fetch repository for {}", name);
                    failed.push(name.clone());
                    continue;
                }
                Err(e) => {
                    eprintln!("Failed to fetch repository for {}: {}", name, e);
                    failed.push(name.clone());
                    continue;
                }
            }
        } else {
            // Not cached, do a fresh clone
            if let Err(e) = try_clone_with_fallback(&repo_cache_dir, git_url) {
                eprintln!("Failed to clone repository for {}: {}", name, e);
                failed.push(name.clone());
                continue;
            }
        }

        // Reset to origin/HEAD to get the latest commit
        let reset_status = Command::new("git")
            .arg("reset")
            .arg("--hard")
            .arg("origin/HEAD")
            .current_dir(&repo_cache_dir)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status();

        match reset_status {
            Ok(s) if s.success() => {},
            Ok(_) => {
                eprintln!("Failed to reset repository for {}", name);
                failed.push(name.clone());
                continue;
            }
            Err(e) => {
                eprintln!("Failed to reset repository for {}: {}", name, e);
                failed.push(name.clone());
                continue;
            }
        }

        // Check the hash of the cached repository
        let cloned_hash = crate::commands::get_git_commit_hash(&repo_cache_dir);

        let has_update = match (&record.last_commit_hash, cloned_hash) {
            (Some(last_hash), Some(new_hash)) => *last_hash != new_hash,
            (None, _) => true, // No previous hash, always update
            (_, None) => true, // Could not get new hash, proceed with update
        };

        if !has_update {
            if verbose {
                println!("{} is already up to date", name);
            }
            continue;
        }

        // Reinstall from the cached directory
        let install_result = crate::commands::install::install(&repo_cache_dir, Some(name.clone()), true, true, verbose, Some(git_url.to_string()));

        match install_result {
            Ok(_) => {
                updated.push(name.clone());
            }
            Err(e) => {
                eprintln!("Failed to reinstall {}: {}", name, e);
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
