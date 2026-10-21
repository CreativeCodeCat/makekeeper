use anyhow::Result;
use clap::Parser;
use serde::Deserialize;

mod cli;
mod commands;
mod config;
mod models;
mod registry;
mod utils;

use cli::{Cli, Action};
use commands::{inspect, install, git_install, list, info, uninstall, update, fetch, sync, search, git_install_impl};
use std::fs;
use std::path::PathBuf;

fn main() {
    if let Err(e) = run() {
        eprintln!("makekeeper: {e:#}");
        std::process::exit(1);
    }
}

fn run() -> Result<()> {
    match Cli::parse().command {
        Action::Inspect { project } => inspect(&project),
        Action::Install { package, name, dir, yes, override_, verbose } => {
            if let Some(ref pkg_name) = package {
                // Package name provided, install from packages.json
                let cache_path = PathBuf::from(config::CACHE_DIR).join("packages.json");
                if cache_path.exists() {
                    let content = fs::read_to_string(&cache_path)?;
                    #[derive(Deserialize)]
                    struct PackageList {
                        #[serde(default)]
                        _version: Option<String>,
                        packages: Vec<Package>,
                    }
                    #[derive(Deserialize)]
                    struct Package {
                        name: String,
                        url: String,
                        description: String,
                        category: String,
                    }
                    let package_list: PackageList = serde_json::from_str(&content)?;
                    
                    if let Some(pkg) = package_list.packages.iter().find(|p| p.name == *pkg_name) {
                        // Found in packages.json, use git install with optional name override
                        git_install_impl(pkg.url.clone(), name.clone(), yes, override_, verbose)
                    } else {
                        anyhow::bail!("Package '{}' not found in package list. Run 'makekeeper search' to find available packages.", pkg_name);
                    }
                } else {
                    anyhow::bail!("Package list not found. Run 'makekeeper sync' first.");
                }
            } else if let Some(project_dir) = dir {
                // --dir provided, install from directory
                install(&project_dir, name, yes, override_, verbose, None)
            } else {
                // No package, install from current directory
                let project_dir = std::env::current_dir()?;
                install(&project_dir, name, yes, override_, verbose, None)
            }
        }
        Action::Git { url, name, yes, override_, verbose } => git_install(url, name, yes, override_, verbose),
        Action::List => list(),
        Action::Info { name } => info(&name),
        Action::Uninstall { name, yes, verbose } => uninstall(&name, yes, verbose),
        Action::Update { yes, verbose } => update(yes, verbose),
        Action::Fetch => fetch(),
        Action::Sync => sync(),
        Action::Search { query } => search(&query),
    }
}
