use async_trait::async_trait;

use crate::{register_service, service::Service, stop_token::StopToken};

pub struct Game {}

register_service!(Game);

#[async_trait]
impl Service for Game {
    fn name(&self) -> &'static str {
        "Game"
    }

    async fn run(&mut self, st: &StopToken) {
        println!("Hello Jungle!");

        st.stop();
    }
}

impl Default for Game {
    fn default() -> Self {
        Self {}
    }
}
