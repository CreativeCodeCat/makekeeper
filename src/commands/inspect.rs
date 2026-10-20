use anyhow::{Context, Result};
use std::collections::BTreeSet;
use std::io::{self, Write};
use std::path::Path;
use std::process::Command;

use crate::utils::project_dir;

pub fn inspect(project: &Path) -> Result<()> {
    let project = project_dir(project)?;
    println!("Project: {}", project.display());

    // -qp prints Make's database. It may return a nonzero status
    // because -q reports whether targets need rebuilding.
    let output = Command::new("make")
        .current_dir(&project)
        .args(["-qp"])
        .output()
        .context("could not run GNU Make")?;

    let mut targets = BTreeSet::new();
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        if line.starts_with('#') || line.starts_with('\t') || line.starts_with(' ') {
            continue;
        }
        if let Some((lhs, rhs)) = line.split_once(':') {
            if !lhs.is_empty() && !lhs.contains(['=', '$', '/', '%', ' ']) && !rhs.starts_with('=')
            {
                targets.insert(lhs.to_string());
            }
        }
    }

    println!("\nPossible targets:");
    for target in targets {
        println!("  {target}");
    }

    println!("\nPreview: make -n install");
    println!("Warning: a Makefile can run arbitrary commands; this is not a sandbox.");

    let preview = Command::new("make")
        .current_dir(&project)
        .args([
            "-n",
            "install",
            "PREFIX=/opt/makekeeper/apps/EXAMPLE",
            "DESTDIR=/tmp/makekeeper-stage",
        ])
        .output()?;

    io::stdout().write_all(&preview.stdout)?;
    io::stderr().write_all(&preview.stderr)?;
    println!("\nReview the Makefile before installing it.");
    Ok(())
}
