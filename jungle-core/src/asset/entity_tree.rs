use std::collections::HashMap;

use bytes::Bytes;

use crate::{asset::env::AssetEnv, ecs::component::ComponentId};

pub enum EntityTree {
    Entity(TagEntity),
    PrefabRef(TagPrefabRef),
    Prefab(TagPrefab),
}

pub struct TagEntity {
    pub entity_id: u64,
    pub name: String,
    pub enabled: bool,
    pub env: AssetEnv,
    pub components: Vec<TagComponent>,
    pub subentities: Vec<EntityTree>,
}

pub struct TagComponent {
    pub component_id: ComponentId,
    pub component: ComponentData,
}

pub struct TagPrefabRef {
    pub name: String,
    pub source: String,
}

pub struct TagPrefab {
    pub name: String,
    pub source: String,
}

pub struct ComponentData(Bytes);

pub type JamlComponent = HashMap<String, String>;
