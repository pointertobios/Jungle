#![allow(unused_imports)]

use jungle_core::{ecs::component::SparseComponentStorage, macros::component};

#[component(storage = SparseComponentStorage, exclusive = false)]
struct WrongEntityType {
    entity: u64,
}

fn main() {}