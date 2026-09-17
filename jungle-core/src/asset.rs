use async_trait::async_trait;

use crate::{register_service, service::Service, stop_token::StopToken};

pub struct Asset {}

register_service!(Asset);

#[async_trait]
impl Service for Asset {
    fn name(&self) -> &'static str {
        "Asset"
    }

    async fn run(&mut self, st: &StopToken) {
        println!("Asset Service");

        let _ = st.stop_requested();
    }
}

impl Default for Asset {
    fn default() -> Self {
        Asset {}
    }
}
