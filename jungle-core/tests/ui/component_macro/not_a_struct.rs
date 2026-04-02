#![allow(unused_imports)]

use jungle_core::{
    ecs::entity::Entity,
    macros::component,
};

#[component(storage = crate::ecs::component::SparseComponentStorage, exclusive = false)]
enum NotAStruct {
    Value { entity: Entity },
}

fn main() {}