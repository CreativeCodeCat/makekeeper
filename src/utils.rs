use anyhow::{Context, Result, bail};
use sha2::{Digest, Sha256};
use std::{
    fs::File,
    io::{Read, Write},
    path::{Path, PathBuf},
    process::{Command, Stdio},
    time::{SystemTime, UNIX_EPOCH},
};

pub fn root_required() -> Result<()> {
    // Check if we've already re-exec'd to avoid loops
    if std::env::var("MAKEKEEPER_PKEXEC").is_ok() {
        return Ok(());
    }

    let out = Command::new("id").arg("-u").output()?;
    if !String::from_utf8_lossy(&out.stdout).trim().eq("0") {
        // Re-exec with pkexec, preserving current directory
        let args: Vec<String> = std::env::args().collect();
        let current_dir = std::env::current_dir()
            .context("failed to get current directory")?;

        // Convert relative paths to absolute in args
        // Skip the subcommand (index 1) and flags (starting with -)
        let absolute_args: Vec<String> = args
            .iter()
            .enumerate()
            .map(|(i, arg)| {
                // Skip converting: the program name (index 0), subcommand (index 1), and flags
                if i <= 1 || arg.starts_with('-') {
                    arg.clone()
                } else if arg.starts_with("./") || arg.starts_with("../") {
                    // Convert relative path to absolute
                    let path = PathBuf::from(arg);
                    current_dir.join(path).to_string_lossy().to_string()
                } else {
                    // Check if it's an existing path (not a name)
                    let path = PathBuf::from(arg);
                    if path.is_relative() && path.exists() {
                        current_dir.join(path).to_string_lossy().to_string()
                    } else {
                        arg.clone()
                    }
                }
            })
            .collect();

        let status = Command::new("pkexec")
            .args(&absolute_args)
            .current_dir(current_dir)
            .env("MAKEKEEPER_PKEXEC", "1")
            .status()
            .context("failed to re-exec with pkexec")?;

        std::process::exit(status.code().unwrap_or(1));
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

pub fn project_dir(path: &Path) -> Result<PathBuf> {
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

pub fn to_https_url(url: &str) -> String {
    // Convert SSH URLs to HTTPS format
    // git@github.com:user/repo.git -> https://github.com/user/repo.git
    // git@gitlab.com:user/repo.git -> https://gitlab.com/user/repo.git
    // etc.

    if url.starts_with("git@") {
        let without_git = url.strip_prefix("git@").unwrap();
        let parts: Vec<&str> = without_git.splitn(2, ':').collect();
        if parts.len() == 2 {
            return format!("https://{}/{}", parts[0], parts[1]);
        }
    }

    // If it's already HTTPS or doesn't match the pattern, return as-is
    url.to_string()
}

pub fn try_clone_with_fallback(temp_dir: &Path, url: &str) -> Result<bool> {
    // Try cloning with the URL as-is first, suppressing all output
    let first_status = Command::new("git")
        .env("GIT_TERMINAL_PROMPT", "0")
        .arg("clone")
        .arg("--quiet")
        .arg(url)
        .arg(temp_dir)
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status();

    if let Ok(status) = first_status {
        if status.success() {
            return Ok(true); // First attempt succeeded
        }
    }

    // If first attempt failed and it's SSH, try HTTPS
    if url.starts_with("git@") {
        let https_url = to_https_url(url);
        let https_status = Command::new("git")
            .env("GIT_TERMINAL_PROMPT", "0")
            .arg("clone")
            .arg("--quiet")
            .arg(&https_url)
            .arg(temp_dir)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
            .context("failed to clone repository with HTTPS")?;

        if https_status.success() {
            return Ok(false); // HTTPS succeeded
        }
    }

    // If first attempt failed and it's git://, try HTTPS
    if url.starts_with("git://") {
        let https_url = url.replace("git://", "https://");
        let https_status = Command::new("git")
            .env("GIT_TERMINAL_PROMPT", "0")
            .arg("clone")
            .arg("--quiet")
            .arg(&https_url)
            .arg(temp_dir)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
            .context("failed to clone repository with HTTPS")?;

        if https_status.success() {
            return Ok(false); // HTTPS succeeded
        }
    }

    anyhow::bail!("failed to clone repository")
}
