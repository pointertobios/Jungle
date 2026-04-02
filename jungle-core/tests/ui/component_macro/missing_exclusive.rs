#![allow(unused_imports)]

use jungle_core::{
    ecs::{component::SparseComponentStorage, entity::Entity},
    macros::component,
};

#[component(storage = SparseComponentStorage)]
struct MissingExclusive {
    entity: Entity,
}

fn main() {}
