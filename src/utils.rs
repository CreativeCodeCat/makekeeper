use anyhow::{Result, bail};
use sha2::{Digest, Sha256};
use std::{
    fs::File,
    io::{Read, Write},
    path::Path,
    process::Command,
    time::{SystemTime, UNIX_EPOCH},
};

pub fn root_required() -> Result<()> {
    let out = Command::new("id").arg("-u").output()?;
    if !String::from_utf8_lossy(&out.stdout).trim().eq("0") {
        bail!("system-wide operation requires root; run this command with sudo");
    }
    Ok(())
}

pub fn valid_name(s: &str) -> bool {
    !s.is_empty()
        && s.len() <= 80
        && s != "."
        && s != ".."
        && s.chars()
            .all(|c| c.is_ascii_alphanumeric() || "._-".contains(c))
}

pub fn project_dir(path: &Path) -> Result<std::path::PathBuf> {
    let p = std::fs::canonicalize(path)?;
    if !p.is_dir() {
        bail!("project path is not a directory");
    }
    if !["Makefile", "makefile", "GNUmakefile"]
        .iter()
        .any(|f| p.join(f).is_file())
    {
        bail!("no Makefile found in {}", p.display());
    }
    Ok(p)
}

pub fn confirm(prompt: &str) -> Result<bool> {
    print!("{prompt}");
    std::io::stdout().flush()?;
    let mut answer = String::new();
    std::io::stdin().read_line(&mut answer)?;
    Ok(matches!(
        answer.trim().to_ascii_lowercase().as_str(),
        "y" | "yes"
    ))
}

pub fn now() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}

pub fn hash_file(path: &Path) -> Result<String> {
    let mut file = File::open(path)?;
    let mut hasher = Sha256::new();
    let mut buffer = [0u8; 16384];

    loop {
        let count = file.read(&mut buffer)?;
        if count == 0 {
            break;
        }
        hasher.update(&buffer[..count]);
    }

    Ok(format!("{:x}", hasher.finalize()))
}
