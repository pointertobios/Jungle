use jungle_macros::component;

use crate::ecs::{
    component::{ComponentExt, SparseComponentStorage},
    entity::Entity,
};

#[component(noserde, storage = SparseComponentStorage, exclusive = true)]
#[derive(Debug, Default)]
pub struct Node {
    entity: Entity,

    name: String,
    enabled: bool,

    parent: Option<Entity>,
    children: Vec<Entity>,
}

impl ComponentExt for Node {}
