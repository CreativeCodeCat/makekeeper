use anyhow::{Context, Result, bail};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

use crate::commands::install;
use crate::utils::try_clone_with_fallback;

fn get_cache_dir() -> PathBuf {
    // Use user-specific cache to avoid permission issues
    let uid = unsafe { libc::getuid() };
    Path::new("/tmp").join(format!("makekeeper-cache-{}", uid))
}

pub fn git_install(url: String, name: Option<String>, yes: bool, override_: bool, verbose: bool) -> Result<()> {
    git_install_impl(url, name, yes, override_, verbose)
}

pub fn git_install_impl(url: String, name: Option<String>, yes: bool, override_: bool, verbose: bool) -> Result<()> {
    // Check if git is available
    let git_check = Command::new("git")
        .arg("--version")
        .output();

    if git_check.is_err() {
        bail!("git is not installed or not available in PATH");
    }

    // Create cache directory
    let cache_dir = get_cache_dir();
    fs::create_dir_all(&cache_dir)
        .context("failed to create cache directory")?;

    // Create a cache directory for this repo
    // Use a sanitized version of the git URL as the cache name
    let cache_name = url
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
            .status();

        if status.is_err() || !status.map(|s| s.success()).unwrap_or(false) {
            // Fetch failed, remove cache and re-clone
            let _ = fs::remove_dir_all(&repo_cache_dir);
            // Fall through to clone below
        } else {
            // Fetch succeeded, proceed to reset
        }
    }

    // Clone if cache doesn't exist or was removed
    if !repo_cache_dir.exists() {
        match try_clone_with_fallback(&repo_cache_dir, &url) {
            Ok(_) => {},
            Err(e) => {
                bail!("failed to clone repository from '{}': {}", url, e);
            }
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

    if reset_status.is_err() || !reset_status.map(|s| s.success()).unwrap_or(false) {
        // Reset failed, remove cache and re-clone
        let _ = fs::remove_dir_all(&repo_cache_dir);
        match try_clone_with_fallback(&repo_cache_dir, &url) {
            Ok(_) => {},
            Err(e) => {
                bail!("failed to clone repository from '{}': {}", url, e);
            }
        }
    }

    // Store the URL as-is (don't convert to HTTPS)
    let url_to_store = url.clone();

    // If no name is provided, try to extract it from the git repository name
    let final_name = if name.is_none() {
        // Extract repository name from URL
        // git@github.com:user/repo.git -> repo
        // https://github.com/user/repo.git -> repo
        let repo_name = if url.contains('/') {
            url.split('/')
                .last()
                .unwrap_or("")
                .trim_end_matches(".git")
                .to_string()
        } else {
            url.trim_end_matches(".git").to_string()
        };

        if repo_name.is_empty() {
            bail!("could not determine repository name from URL; please provide --name");
        }

        Some(repo_name)
    } else {
        name
    };

    // Call install with the cached directory
    // The install command will check for root privileges
    let result = install::install(repo_cache_dir.as_path(), final_name, yes, override_, verbose, Some(url_to_store));

    // Don't clean up the cache - it's shared with update

    result
}
