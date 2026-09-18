use async_trait::async_trait;

use crate::{register_service, service::Service, service_controller::ServiceController};

pub struct Game {}

register_service!(Game);

#[async_trait]
impl Service for Game {
    fn name(&self) -> &'static str {
        "Game"
    }

    async fn run(&mut self, service_ctl: &ServiceController) {
        println!("Hello Jungle!");

        service_ctl.stop();
    }
}

impl Default for Game {
    fn default() -> Self {
        Self {}
    }
}
