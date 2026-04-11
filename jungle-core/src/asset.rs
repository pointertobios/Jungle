use std::{
    collections::HashMap,
    num::NonZeroU128,
    path::{Path, PathBuf},
    sync::Arc,
};

use anyhow::{Context, Result};
use blake3::hash;
use bytes::Bytes;
use serde::{Deserialize, Serialize};
use tokio::fs;

pub mod entity_tree;
pub mod env;

#[derive(Debug, Default, Clone, Copy, PartialEq, Eq, Hash)]
pub struct AssetId(Option<NonZeroU128>);

impl AssetId {
    pub fn from_path(path: &PathBuf) -> Self {
        let digest = hash(path.as_os_str().as_encoded_bytes());
        let mut bytes = [0_u8; 16];
        bytes.copy_from_slice(&digest.as_bytes()[..16]);
        Self(NonZeroU128::new(u128::from_le_bytes(bytes)))
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub struct AssetMetadata {
    pub initial_scene: PathBuf,
}

impl AssetMetadata {
    pub async fn from_filesystem_root(asset_root: &Path) -> Result<Self> {
        let metadata_path = asset_root.join("metadata.toml");
        let metadata_source = fs::read_to_string(&metadata_path).await.with_context(|| {
            format!("failed to read asset metadata {}", metadata_path.display())
        })?;

        toml::from_str(&metadata_source).with_context(|| {
            format!(
                "failed to deserialize asset metadata from {}",
                metadata_path.display()
            )
        })
    }
}

#[derive(Debug)]
pub enum AssetPackage {
    FileSystem(PathBuf),
}

#[derive(Debug)]
pub enum Asset {
    Unloaded(PathBuf),
    Loading,
    Loaded(PathBuf, Arc<Bytes>),
    Unloading,
    Failed(PathBuf, anyhow::Error),
}

#[derive(Debug)]
pub struct AssetManager {
    pkg: AssetPackage,
    meta: AssetMetadata,

    map: Arc<HashMap<AssetId, Arc<Asset>>>,
}

impl AssetManager {
    pub async fn new(pkg: AssetPackage) -> Result<Self> {
        let meta = match &pkg {
            AssetPackage::FileSystem(root) => AssetMetadata::from_filesystem_root(root).await?,
        };
        let mut map = HashMap::new();
        match &pkg {
            AssetPackage::FileSystem(root) => {
                Self::scan_filesystem_package(root, &mut map)
                    .await
                    .with_context(|| format!("failed to scan asset root {}", root.display()))?
            }
        }

        Ok(Self {
            pkg,
            meta,
            map: Arc::new(map),
        })
    }

    async fn scan_filesystem_package(
        asset_root: &Path,
        map: &mut HashMap<AssetId, Arc<Asset>>,
    ) -> Result<()> {
        let mut pending_dirs = vec![asset_root.to_path_buf()];

        while let Some(current_dir) = pending_dirs.pop() {
            let mut entries = fs::read_dir(&current_dir)
                .await
                .with_context(|| format!("failed to read directory {}", current_dir.display()))?;

            while let Some(entry) = entries
                .next_entry()
                .await
                .with_context(|| format!("failed to iterate directory {}", current_dir.display()))?
            {
                let path = entry.path();
                let file_type = entry
                    .file_type()
                    .await
                    .with_context(|| format!("failed to inspect {}", path.display()))?;

                if file_type.is_dir() {
                    pending_dirs.push(path);
                    continue;
                }

                if !file_type.is_file() {
                    continue;
                }

                if current_dir == asset_root
                    && path
                        .file_name()
                        .is_some_and(|file_name| file_name == "metadata.toml")
                {
                    continue;
                }

                let relative_path = path
                    .strip_prefix(asset_root)
                    .with_context(|| {
                        format!(
                            "failed to compute relative asset path for {} against {}",
                            path.display(),
                            asset_root.display()
                        )
                    })?
                    .to_path_buf();

                map.insert(
                    AssetId::from_path(&relative_path),
                    Arc::new(Asset::Unloaded(path)),
                );
            }
        }

        Ok(())
    }
}
