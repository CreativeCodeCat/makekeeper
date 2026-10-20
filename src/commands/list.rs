use anyhow::Result;
use std::path::Path;

use crate::config::REGISTRY;
use crate::models::Record;
use crate::utils::root_required;

pub fn list() -> Result<()> {
    root_required()?;
    if !Path::new(REGISTRY).is_dir() {
        println!("No registered applications.");
        return Ok(());
    }

    let mut found = false;
    for entry in std::fs::read_dir(REGISTRY)? {
        let path = entry?.path();
        if path.extension().is_none_or(|e| e != "json") {
            continue;
        }
        let record: Record = serde_json::from_slice(&std::fs::read(path)?)?;
        println!(
            "{:<24} {} files, {} links",
            record.name,
            record.files.len(),
            record.links.len()
        );
        found = true;
    }

    if !found {
        println!("No registered applications.");
    }
    Ok(())
}
