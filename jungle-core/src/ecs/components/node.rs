use jungle_macros::component;

use crate::ecs::{
    component::{ComponentExt, SparseComponentStorage},
    entity::Entity,
};

#[component(storage = SparseComponentStorage, exclusive = true)]
#[derive(Debug)]
pub struct Node {
    entity: Entity,

    name: String,
    enabled: bool,

    parent: Option<Entity>,
    children: Vec<Entity>,
}

impl ComponentExt for Node {}

impl Node {
    pub fn new(entity: Entity, name: String) -> Self {
        Self {
            entity,
            name,
            enabled: true,
            parent: None,
            children: Vec::new(),
        }
    }
}

impl Default for Node {
    fn default() -> Self {
        Self {
            entity: Entity::default(),
            name: String::new(),
            enabled: true,
            parent: None,
            children: Vec::new(),
        }
    }
}
