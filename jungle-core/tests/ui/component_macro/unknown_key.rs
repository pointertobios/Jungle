#![allow(unused_imports)]

use jungle_core::{
    ecs::{component::SparseComponentStorage, entity::Entity},
    macros::component,
};

#[component(storage = SparseComponentStorage, exclusive = false, exclusiv = true)]
struct UnknownKey {
    entity: Entity,
}

fn main() {}