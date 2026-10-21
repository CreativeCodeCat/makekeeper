use anyhow::{Result, bail};
use serde_json;
use std::path::{Path, PathBuf};

use crate::config::{APP_ROOT, BIN_DIR, MAN_DIR, REGISTRY};
use crate::models::{Record, InstalledFile, ManagedLink};
use crate::utils::valid_name;

pub fn record_path(name: &str) -> PathBuf {
    Path::new(REGISTRY).join(format!("{name}.json"))
}

pub fn load_record(name: &str) -> Result<Record> {
    if !valid_name(name) {
        bail!("invalid application name");
    }

    let path = record_path(name);
    if !path.exists() {
        bail!("'{name}' is not installed");
    }

    let data = std::fs::read(&path)?;
    let record: Record = serde_json::from_slice(&data)?;

    let expected_prefix = Path::new(APP_ROOT).join(name);
    if record.name != name || Path::new(&record.prefix) != expected_prefix {
        bail!("invalid registry record; refusing to operate");
    }

    for file in &record.files {
        if !Path::new(&file.path).starts_with(&expected_prefix) {
            bail!("unsafe file path in registry");
        }
    }

    for link in &record.links {
        let is_safe_path = Path::new(&link.path).starts_with(BIN_DIR)
            || Path::new(&link.path).starts_with(MAN_DIR);
        if !is_safe_path || !Path::new(&link.target).starts_with(&expected_prefix) {
            bail!("unsafe symlink record");
        }
    }

    Ok(record)
}

pub fn save_record(record: &Record) -> Result<()> {
    std::fs::create_dir_all(REGISTRY)?;
    let path = record_path(&record.name);
    let temp = path.with_extension(format!("tmp-{}", std::process::id()));
    std::fs::write(&temp, serde_json::to_vec_pretty(record)?)?;
    std::fs::rename(temp, path)?;
    Ok(())
}

pub fn get_all_records() -> Result<Vec<Record>> {
    let mut records = Vec::new();

    if !Path::new(REGISTRY).exists() {
        return Ok(records);
    }

    for entry in std::fs::read_dir(REGISTRY)? {
        let entry = entry?;
        let path = entry.path();

        if path.extension().and_then(|s| s.to_str()) != Some("json") {
            continue;
        }

        let name = path
            .file_stem()
            .and_then(|s| s.to_str())
            .ok_or_else(|| anyhow::anyhow!("invalid record filename"))?;

        let record = load_record(name)?;
        records.push(record);
    }

    Ok(records)
}

pub fn rollback_files(files: &[InstalledFile]) {
    for file in files.iter().rev() {
        let path = Path::new(&file.path);
        let remove = if let Some(expected) = &file.sha256 {
            crate::utils::hash_file(path)
                .map(|actual| actual == *expected)
                .unwrap_or(false)
        } else if let Some(expected) = &file.link_target {
            std::fs::read_link(path)
                .map(|actual| actual.to_string_lossy() == *expected)
                .unwrap_or(false)
        } else {
            false
        };

        if remove {
            let _ = std::fs::remove_file(path);
        }
    }
}

pub fn rollback_links(links: &[ManagedLink]) {
    for link in links.iter().rev() {
        let path = Path::new(&link.path);
        if std::fs::read_link(path)
            .map(|target| target.to_string_lossy() == link.target)
            .unwrap_or(false)
        {
            let _ = std::fs::remove_file(path);
        }
    }
}
