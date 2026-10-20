use anyhow::{Context, Result, bail};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

use crate::commands::install;
use crate::utils::try_clone_with_fallback;

fn get_cache_dir() -> PathBuf {
    Path::new("/tmp").join("makekeeper-cache")
}

pub fn git_install(url: String, name: Option<String>, yes: bool, override_: bool, verbose: bool) -> Result<()> {
    // Check if git is available
    let git_check = Command::new("git")
        .arg("--version")
        .output();

    if git_check.is_err() {
        bail!("git is not installed or not available in PATH");
    }

    let sudo_user = std::env::var("SUDO_USER").ok();
    let ssh_auth_sock = std::env::var("SSH_AUTH_SOCK").ok();

    // Create cache directory
    let cache_dir = get_cache_dir();
    if let Some(ref user) = sudo_user {
        // Create as the user to avoid permission issues
        Command::new("sudo")
            .arg("-u")
            .arg(user)
            .arg("mkdir")
            .arg("-p")
            .arg(&cache_dir)
            .status()
            .context("failed to create cache directory")?;
    } else {
        fs::create_dir_all(&cache_dir)
            .context("failed to create cache directory")?;
    }

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
        if let Some(ref user) = sudo_user {
            let mut fetch_cmd = Command::new("sudo");
            fetch_cmd
                .arg("-u")
                .arg(user)
                .arg("git")
                .arg("fetch")
                .arg("--quiet")
                .current_dir(&repo_cache_dir)
                .stdout(Stdio::null())
                .stderr(Stdio::null())
                .env("HOME", format!("/home/{}", user));

            if let Some(ref sock) = ssh_auth_sock {
                fetch_cmd.env("SSH_AUTH_SOCK", sock);
            }

            let status = fetch_cmd
                .status()
                .context("failed to fetch repository")?;

            if !status.success() {
                bail!("failed to fetch repository");
            }
        } else {
            let status = Command::new("git")
                .arg("fetch")
                .arg("--quiet")
                .current_dir(&repo_cache_dir)
                .stdout(Stdio::null())
                .stderr(Stdio::null())
                .status()
                .context("failed to fetch repository")?;

            if !status.success() {
                bail!("failed to fetch repository");
            }
        }
    } else {
        // Not cached, do a fresh clone
        if let Some(ref user) = sudo_user {
            let mut clone_cmd = Command::new("sudo");
            clone_cmd
                .arg("-u")
                .arg(user)
                .arg("git")
                .arg("clone")
                .arg("--quiet")
                .arg(&url)
                .arg(&repo_cache_dir)
                .stdout(Stdio::null())
                .stderr(Stdio::null())
                .env("HOME", format!("/home/{}", user));

            if let Some(ref sock) = ssh_auth_sock {
                clone_cmd.env("SSH_AUTH_SOCK", sock);
            }

            let status = clone_cmd
                .status()
                .context("failed to clone repository")?;

            if !status.success() {
                bail!("failed to clone repository");
            }
        } else {
            try_clone_with_fallback(&repo_cache_dir, &url)?;
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
        .status()
        .context("failed to reset repository")?;

    if !reset_status.success() {
        bail!("failed to reset repository");
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
