use anyhow::Result;
use clap::Parser;

mod cli;
mod commands;
mod config;
mod models;
mod registry;
mod utils;

use cli::{Cli, Action};
use commands::{inspect, install, git_install, list, info, uninstall, update, fetch, sync};

fn main() {
    if let Err(e) = run() {
        eprintln!("makekeeper: {e:#}");
        std::process::exit(1);
    }
}

fn run() -> Result<()> {
    match Cli::parse().command {
        Action::Inspect { project } => inspect(&project),
        Action::Install { project, name, yes, override_, verbose } => install(&project, name, yes, override_, verbose, None),
        Action::Git { url, name, yes, override_, verbose } => git_install(url, name, yes, override_, verbose),
        Action::List => list(),
        Action::Info { name } => info(&name),
        Action::Uninstall { name, yes, verbose } => uninstall(&name, yes, verbose),
        Action::Update { yes, verbose } => update(yes, verbose),
        Action::Fetch => fetch(),
        Action::Sync => sync(),
    }
}
