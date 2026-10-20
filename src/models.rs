use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
pub struct Record {
    pub name: String,
    pub source: String,
    pub prefix: String,
    pub installed_at: u64,
    pub files: Vec<InstalledFile>,
    pub links: Vec<ManagedLink>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub git_url: Option<String>,
}

#[derive(Serialize, Deserialize)]
pub struct InstalledFile {
    pub path: String,
    pub sha256: Option<String>,
    pub link_target: Option<String>,
}

#[derive(Serialize, Deserialize)]
pub struct ManagedLink {
    pub path: String,
    pub target: String,
}
