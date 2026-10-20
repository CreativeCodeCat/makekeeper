use anyhow::{Context, Result, bail};
use std::env;
use std::fs;
use std::process::Command;

use crate::commands::install;
use crate::utils::root_required;

pub fn git_install(url: String, name: Option<String>, yes: bool) -> Result<()> {
    // Check for root privileges
    root_required()?;

    // Check if git is available
    let git_check = Command::new("git")
        .arg("--version")
        .output();

    if git_check.is_err() {
        bail!("git is not installed or not available in PATH");
    }

    // Create a temporary directory for cloning
    let temp_dir = env::temp_dir().join(format!("makekeeper-git-{}", std::process::id()));
    fs::create_dir_all(&temp_dir)
        .context("failed to create temporary directory for git clone")?;

    // Clone the repository
    println!("Cloning repository from {}", url);
    let clone_status = Command::new("git")
        .arg("clone")
        .arg(&url)
        .arg(&temp_dir)
        .status()
        .context("failed to clone repository")?;

    if !clone_status.success() {
        fs::remove_dir_all(&temp_dir)?;
        bail!("git clone failed");
    }

    println!("Repository cloned to {}", temp_dir.display());

    // Call install with the cloned directory
    let result = install::install(temp_dir.as_path(), name, yes);

    // Clean up the cloned directory
    let _ = fs::remove_dir_all(&temp_dir);

    result
}
