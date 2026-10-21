use clap::{Parser, Subcommand};
use std::path::PathBuf;

#[derive(Parser)]
#[command(
    version,
    about = "A safer installer for Makefile-based Linux applications"
)]
pub struct Cli {
    #[command(subcommand)]
    pub command: Action,
}

#[derive(Subcommand)]
pub enum Action {
    /// Inspect a project's Makefile and preview installation commands.
    Inspect { project: PathBuf },
    /// Build and install a project.
    Install {
        project: PathBuf,
        #[arg(long)]
        name: Option<String>,
        #[arg(long, help = "Skip confirmation prompt")]
        yes: bool,
        #[arg(long, help = "Override existing installation")]
        override_: bool,
        #[arg(long, help = "Show detailed installation output")]
        verbose: bool,
    },
    /// Clone a git repository and install it.
    Git {
        url: String,
        #[arg(long)]
        name: Option<String>,
        #[arg(long, help = "Skip confirmation prompt")]
        yes: bool,
        #[arg(long, help = "Override existing installation")]
        override_: bool,
        #[arg(long, help = "Show detailed installation output")]
        verbose: bool,
    },
    /// List registered applications.
    List,
    /// Display an installation record.
    Info { name: String },
    /// Remove tracked files and managed executable symlinks.
    Uninstall {
        name: String,
        #[arg(long, help = "Skip confirmation prompt")]
        yes: bool,
        #[arg(long, help = "Show detailed removal output")]
        verbose: bool,
    },
    /// Update all installed applications that have git repositories.
    Update {
        #[arg(long, help = "Skip confirmation prompt")]
        yes: bool,
        #[arg(long, help = "Show detailed update output")]
        verbose: bool,
    },
    /// Show count of installed packages and git-tracked packages.
    Fetch,
    /// Sync package list from remote repository.
    Sync,
}
