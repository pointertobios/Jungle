use jungle_macros::component;

use crate::{
    asset::AssetId,
    ecs::{
        component::{ComponentExt, SparseComponentStorage},
        entity::Entity,
    },
};

#[component(noserde, storage = SparseComponentStorage, exclusive = true)]
#[derive(Debug, Default)]
pub struct PrefabRef {
    entity: Entity,

    name: String,
    asset_id: AssetId,
}

impl ComponentExt for PrefabRef {}
