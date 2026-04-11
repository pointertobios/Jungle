use std::path::PathBuf;

use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
pub struct Project {
    pub name: String,
    pub assets: PathBuf,
}

impl Project {
    pub async fn from(project_dir: &PathBuf) -> Result<Self> {
        let proj_path = project_dir.join("jungle.proj.toml");
        let proj_source = tokio::fs::read_to_string(&proj_path)
            .await
            .with_context(|| format!("failed to read project file {}", proj_path.display()))?;

        toml::from_str(&proj_source)
            .with_context(|| format!("failed to deserialize project from {}", proj_path.display()))
    }
}
