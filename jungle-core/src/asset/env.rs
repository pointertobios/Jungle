use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq)]
pub enum AssetEnv {
    Client, // client_only="true"
    Server, // server_only="true"
    Both,   // default
}

impl Default for AssetEnv {
    fn default() -> Self {
        AssetEnv::Both
    }
}
