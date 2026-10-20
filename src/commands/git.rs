use anyhow::{Context, Result, bail};
use std::env;
use std::fs;
use std::path::Path;
use std::process::{Command, Stdio};

use crate::commands::install;
use crate::utils::try_clone_with_fallback;

pub fn git_install(url: String, name: Option<String>, yes: bool, override_: bool, verbose: bool) -> Result<()> {
    // Check if git is available
    let git_check = Command::new("git")
        .arg("--version")
        .output();

    if git_check.is_err() {
        bail!("git is not installed or not available in PATH");
    }

    // Create a temporary directory for cloning
    // Use /tmp when running under sudo to avoid permission issues
    let sudo_user = std::env::var("SUDO_USER").ok();
    let temp_dir = if sudo_user.is_some() {
        Path::new("/tmp").join(format!("makekeeper-git-{}", std::process::id()))
    } else {
        env::temp_dir().join(format!("makekeeper-git-{}", std::process::id()))
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
            .context("failed to create temporary directory for git clone")?;
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
            .arg(&url)
            .arg(&temp_dir)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
            .context("failed to clone repository")?;

        if !status.success() {
            fs::remove_dir_all(&temp_dir)?;
            bail!("failed to clone repository");
        }
    } else {
        // Not running under sudo - use fallback logic
        try_clone_with_fallback(&temp_dir, &url)?;
    }

    // Store the URL as-is (don't convert to HTTPS)
    let url_to_store = url.clone();

    // Call install with the cloned directory
    // The install command will check for root privileges
    let result = install::install(temp_dir.as_path(), name, yes, override_, verbose, Some(url_to_store));

    // Clean up the cloned directory
    let _ = fs::remove_dir_all(&temp_dir);

    result
}
