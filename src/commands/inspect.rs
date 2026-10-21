use anyhow::{Context, Result};
use std::collections::BTreeSet;
use std::io::{self, Write};
use std::path::Path;
use std::process::Command;

use crate::utils::project_dir;

fn is_valid_target(target: &str) -> bool {
    !target.is_empty()
        && !target.contains(['=', '$', '/', '%', ' '])
        && !target.starts_with('.')
}

fn extract_targets(make_output: &str) -> BTreeSet<String> {
    let mut targets = BTreeSet::new();

    for line in make_output.lines() {
        // Skip comments, tabs, and space-prefixed lines
        if line.starts_with('#') || line.starts_with('\t') || line.starts_with(' ') {
            continue;
        }

        if let Some((lhs, rhs)) = line.split_once(':') {
            if is_valid_target(lhs) && !rhs.starts_with('=') {
                targets.insert(lhs.to_string());
            }
        }
    }

    targets
}

pub fn inspect(project: &Path) -> Result<()> {
    let project = project_dir(project)?;
    println!("Project: {}", project.display());

    // Extract project name from path for preview
    let app_name = project
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or("APPNAME");

    // Get Make database (may return nonzero status due to -q flag)
    let output = Command::new("make")
        .current_dir(&project)
        .args(["-qp"])
        .output()
        .context("could not run GNU Make")?;

    let targets = extract_targets(&String::from_utf8_lossy(&output.stdout));

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
            &format!("PREFIX=/opt/makekeeper/apps/{}", app_name),
        ])
        .output()?;

    io::stdout().write_all(&preview.stdout)?;
    io::stderr().write_all(&preview.stderr)?;
    println!("\nReview the Makefile before installing it.");
    Ok(())
}
