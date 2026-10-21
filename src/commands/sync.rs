use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;

use crate::config::CACHE_DIR;
use crate::utils::root_required;

const PACKAGES_URL: &str = "https://creativecodecat.github.io/makekeeper/packages.json";

#[derive(Debug, Serialize, Deserialize)]
struct PackageList {
    #[serde(default)]
    version: Option<String>,
    packages: Vec<Package>,
}

#[derive(Debug, Serialize, Deserialize)]
struct Package {
    name: String,
    url: String,
    description: String,
    category: String,
}

pub fn sync() -> Result<()> {
    root_required()?;
    let cache_path = PathBuf::from(CACHE_DIR).join("packages.json");

    // Create cache directory if it doesn't exist
    if let Some(parent) = cache_path.parent() {
        fs::create_dir_all(parent)
            .context("failed to create cache directory")?;
    }

    // Download the remote packages.json
    let response = reqwest::blocking::get(PACKAGES_URL)
        .context("failed to download packages.json")?;

    if !response.status().is_success() {
        anyhow::bail!("failed to download packages.json: HTTP {}", response.status());
    }

    let remote_content = response.text()
        .context("failed to read packages.json content")?;

    // Parse the remote version
    let remote_list: PackageList = serde_json::from_str(&remote_content)
        .context("failed to parse remote packages.json")?;

    let remote_version = remote_list.version.clone();

    // Check if we have a cached version
    let needs_update = if cache_path.exists() {
        let cached_content = fs::read_to_string(&cache_path)
            .context("failed to read cached packages.json")?;

        let cached_list: PackageList = serde_json::from_str(&cached_content)
            .context("failed to parse cached packages.json")?;

        // Compare versions (if both have versions)
        match (cached_list.version, remote_version.clone()) {
            (Some(cached), Some(remote)) => cached != remote,
            _ => true, // If either is missing, update
        }
    } else {
        true // No cache, needs update
    };

    if needs_update {
        // Write the new version to cache
        fs::write(&cache_path, remote_content)
            .context("failed to write packages.json to cache")?;

        if let Some(ref v) = remote_version {
            println!("Updated package list to version {}", v);
        } else {
            println!("Updated package list");
        }
    } else {
        if let Some(ref v) = remote_version {
            println!("Package list is already up to date (version {})", v);
        } else {
            println!("Package list is already up to date");
        }
    }

    Ok(())
}
