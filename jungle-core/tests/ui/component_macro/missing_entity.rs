#![allow(unused_imports)]

use jungle_core::{ecs::component::SparseComponentStorage, macros::component};

#[component(storage = SparseComponentStorage, exclusive = false)]
struct MissingEntity {
    value: u32,
}

fn main() {}