use crate::ecs::{component::Component, entity::Entity};

pub trait ComponentStorage<T: Component> {
    fn get_components<'a>(&'a self) -> impl Iterator<Item = &'a T>
    where
        T: 'a;
    fn get_components_mut<'a>(&'a mut self) -> impl Iterator<Item = &'a mut T>
    where
        T: 'a;

    fn get_component(&self, entity: Entity) -> &T;
    fn get_component_mut(&mut self, entity: Entity) -> &mut T;
}
