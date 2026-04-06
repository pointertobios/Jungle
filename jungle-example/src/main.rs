use std::time::Duration;

use jungle_core::game::{Game, GameCore};

fn main() -> anyhow::Result<()> {
    let game = GameCore::new(Duration::from_millis(10));
    game.run()
}
