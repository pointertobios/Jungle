use async_trait::async_trait;

use crate::{register_service, service::Service, service_controller::ServiceController};

pub struct Asset {}

register_service!(Asset);

#[async_trait]
impl Service for Asset {
    fn name(&self) -> &'static str {
        "Asset"
    }

    async fn run(&mut self, service_ctl: &ServiceController) {
        println!("Asset Service");

        service_ctl.wait_for_stop().await;
    }
}

impl Default for Asset {
    fn default() -> Self {
        Asset {}
    }
}
