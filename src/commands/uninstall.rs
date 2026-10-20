use anyhow::Result;
use std::fs;
use std::io;
use std::path::Path;
use walkdir::WalkDir;

use crate::registry::{load_record, record_path};
use crate::utils::{confirm, hash_file, root_required};

pub fn uninstall(name: &str, yes: bool, verbose: bool) -> Result<()> {
    root_required()?;
    let record = load_record(name)?;

    if verbose {
        println!("Application: {name}");
        println!("Tracked files: {}", record.files.len());
        println!("Symlinks: {}", record.links.len());
    }

    if !yes && !confirm("Remove unchanged tracked files? [y/N] ")? {
        println!("Cancelled.");
        return Ok(());
    }

    let mut skipped = false;

    for link in record.links.iter().rev() {
        let path = Path::new(&link.path);
        match fs::read_link(path) {
            Ok(target) if target.to_string_lossy() == link.target => {
                fs::remove_file(path)?;
                if verbose {
                    println!("Removed link {}", path.display());
                }
            }
            Ok(_) => {
                if verbose {
                    eprintln!("Keeping modified symlink {}", path.display());
                }
                skipped = true;
            }
            Err(e) if e.kind() == io::ErrorKind::NotFound => {}
            Err(e) => return Err(e.into()),
        }
    }

    for file in record.files.iter().rev() {
        let path = Path::new(&file.path);
        let meta = match fs::symlink_metadata(path) {
            Ok(m) => m,
            Err(e) if e.kind() == io::ErrorKind::NotFound => continue,
            Err(e) => return Err(e.into()),
        };

        let unchanged = if meta.file_type().is_symlink() {
            file.link_target
                .as_ref()
                .and_then(|expected| {
                    fs::read_link(path)
                        .ok()
                        .map(|actual| actual.to_string_lossy() == *expected)
                })
                .unwrap_or(false)
        } else if meta.is_file() {
            file.sha256
                .as_ref()
                .and_then(|expected| hash_file(path).ok().map(|actual| actual == *expected))
                .unwrap_or(false)
        } else {
            false
        };

        if unchanged {
            fs::remove_file(path)?;
            if verbose {
                println!("Removed {}", path.display());
            }
        } else {
            if verbose {
                eprintln!("Keeping changed or unexpected file {}", path.display());
            }
            skipped = true;
        }
    }

    // Remove only empty directories inside this application's prefix.
    let prefix = Path::new(&record.prefix);
    if prefix.is_dir() {
        for entry in WalkDir::new(prefix)
            .contents_first(true)
            .into_iter()
            .filter_map(Result::ok)
        {
            if entry.file_type().is_dir() {
                let _ = fs::remove_dir(entry.path());
            }
        }
        let _ = fs::remove_dir(prefix);
    }

    if skipped {
        if verbose {
            println!("Some items were retained. Registry kept for manual review.");
        }
    } else {
        fs::remove_file(record_path(name))?;
        if verbose {
            println!("Uninstalled {name}.");
        }
    }
    Ok(())
}
