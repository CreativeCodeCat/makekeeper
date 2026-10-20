use anyhow::Result;
use clap::Parser;

mod cli;
mod commands;
mod config;
mod models;
mod registry;
mod utils;

use cli::{Cli, Action};
use commands::{inspect, install, git_install, list, info, uninstall};

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
        Action::Git { url, name, yes } => git_install(url, name, yes),
        Action::List => list(),
        Action::Info { name } => info(&name),
        Action::Uninstall { name, yes } => uninstall(&name, yes),
    }
}
