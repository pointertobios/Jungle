use std::path::PathBuf;

use jungle_core::game::{CoreConfig, Game, GameCore};

fn main() -> anyhow::Result<()> {
    let game = GameCore::new(CoreConfig {
        project_dir: PathBuf::from("./jungle-example"),
        ..CoreConfig::default()
    });
    game.run()
}
