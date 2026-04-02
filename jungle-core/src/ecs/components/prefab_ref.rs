use jungle_macros::component;

use crate::{
    asset::AssetId,
    ecs::{
        component::{ComponentExt, SparseComponentStorage},
        entity::Entity,
    },
};

#[component(storage = SparseComponentStorage, exclusive = true)]
pub struct PrefabRef {
    entity: Entity,

    name: String,
    asset_id: AssetId,
}

impl ComponentExt for PrefabRef {}

impl PrefabRef {
    pub fn new() -> Self {
        Self {
            entity: Entity::default(),
            name: String::new(),
            asset_id: AssetId::default(),
        }
    }
}

impl Default for PrefabRef {
    fn default() -> Self {
        Self::new()
    }
}
