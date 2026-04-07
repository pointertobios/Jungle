#![allow(unused_imports)]

use jungle_core::{
    ecs::{component::SparseComponentStorage, entity::Entity},
    macros::component,
};

#[component(storage = SparseComponentStorage, exclusive = true)]
struct InvalidSerdeArgs {
    entity: Entity,
    #[serde(u32, |value| *value, |value| *value)]
    value: u32,
}

fn main() {}