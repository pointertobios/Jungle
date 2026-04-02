#![allow(unused_imports)]

use jungle_core::{
    ecs::{component::SparseComponentStorage, entity::Entity},
    macros::component,
};

#[component(storage = SparseComponentStorage<Self>, exclusive = false)]
struct StorageWithGenerics {
    entity: Entity,
}

fn main() {}
