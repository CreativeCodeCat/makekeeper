use anyhow::{Context, Result};
use colored::Colorize;
use serde::Deserialize;
use std::fs;
use std::path::PathBuf;

use crate::config::CACHE_DIR;

#[derive(Debug, Deserialize)]
struct PackageList {
    #[serde(default)]
    _version: Option<String>,
    packages: Vec<Package>,
}

#[derive(Debug, Deserialize)]
struct Package {
    name: String,
    url: String,
    description: String,
    category: String,
}

pub fn search(query: &str) -> Result<()> {
    let cache_path = PathBuf::from(CACHE_DIR).join("packages.json");

    if !cache_path.exists() {
        eprintln!("Package list not found. Run 'makekeeper sync' first.");
        return Ok(());
    }

    let content = fs::read_to_string(&cache_path)
        .context("failed to read cached packages.json")?;

    let package_list: PackageList = serde_json::from_str(&content)
        .context("failed to parse cached packages.json")?;

    let query_lower = query.to_lowercase();
    let mut found = false;

    for package in &package_list.packages {
        let matches = package.name.to_lowercase().contains(&query_lower)
            || package.description.to_lowercase().contains(&query_lower)
            || package.category.to_lowercase().contains(&query_lower)
            || package.url.to_lowercase().contains(&query_lower);

        if matches {
            println!("{} ({})", package.name.green(), package.category.green());
            println!("  {}", package.description.yellow());
            println!("  {}", package.url);
            println!();
            found = true;
        }
    }

    if !found {
        println!("No packages found matching '{}'", query);
    }

    Ok(())
}
