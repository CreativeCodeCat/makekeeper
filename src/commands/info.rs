use anyhow::Result;

use crate::registry::load_record;
use crate::utils::root_required;

pub fn info(name: &str) -> Result<()> {
    root_required()?;
    let record = load_record(name)?;
    println!("Name:   {}", record.name);
    println!("Source: {}", record.source);
    println!("Prefix: {}", record.prefix);
    println!("Files:");
    for file in &record.files {
        println!("  {}", file.path);
    }
    println!("Symlinks:");
    for link in &record.links {
        println!("  {} -> {}", link.path, link.target);
    }
    Ok(())
}
