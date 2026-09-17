use jungle_base::StringID;
use jungle_core::application::Application;

#[tokio::main]
async fn main() {
    let mut app = Application::new([StringID::of("Game")]);
    app.run().await
}
