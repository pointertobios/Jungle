use std::ops::{Index, IndexMut};

use crate::ecs::entity::Entity;

pub const COMPONENT_GROUP_SIZE: usize = 16;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ComponentID {
    id: u64,
}

pub trait Component: Default {
    type SoAType;

    fn name() -> &'static str;
}

pub struct SingleComponent<T: Component> {
    id: ComponentID,
    owner: Entity,
    component: T,
}

pub struct AoSComponentGroup<T: Component> {
    ids: [ComponentID; COMPONENT_GROUP_SIZE],
    owners: [Entity; COMPONENT_GROUP_SIZE],
    components: [T; COMPONENT_GROUP_SIZE],
}

pub struct SoAComponentGroup<T: Component> {
    ids: [ComponentID; COMPONENT_GROUP_SIZE],
    owners: [Entity; COMPONENT_GROUP_SIZE],
    components: T::SoAType,
}
