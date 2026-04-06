#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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
