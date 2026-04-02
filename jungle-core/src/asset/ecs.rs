use spex::xml::XmlDocument;

use crate::{asset::env::AssetEnv, ecs::entity::Entity};

async fn parse_entity(env: AssetEnv, doc: XmlDocument) -> anyhow::Result<(Entity, Vec<Entity>)> {
    todo!()
}
