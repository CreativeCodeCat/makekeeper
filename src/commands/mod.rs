pub mod inspect;
pub mod install;
pub mod git;
pub mod list;
pub mod info;
pub mod uninstall;
pub mod update;

pub use inspect::inspect;
pub use install::{install, get_git_commit_hash};
pub use git::git_install;
pub use list::list;
pub use info::info;
pub use uninstall::uninstall;
pub use update::update;
