use std::{
    any::{Any, TypeId},
    collections::{HashMap, HashSet},
};

use async_trait::async_trait;
use itertools::Either;
use tokio::sync::mpsc::{Receiver, Sender};

use crate::ecs::entity::Entity;

pub trait Component: 'static + ComponentExt + Default + Sized + Send + Sync {
    type Storage: ComponentStorage<Self>;

    const EXCLUSIVE: bool; // 互斥，每个 Entity 最多只能有一个该 Component 类型的组件

    fn type_id(&self) -> TypeId;
    fn type_name(&self) -> &'static str;
    fn downcast_unchecked<T>(&self) -> &T;
    fn downcast_unchecked_mut<T>(&mut self) -> &mut T;

    fn entity(&self) -> Entity;
}

pub trait ComponentExt {
    fn initialize(&mut self) {}
    fn finalize(&mut self) {}

    fn pre_tick(&mut self) {}
    fn tick(&mut self) {}
    fn post_tick(&mut self) {}
}

#[async_trait]
pub trait ComponentManager {
    fn get_component_type_id(&self) -> TypeId;
    fn as_any(&self) -> &dyn Any;
    fn as_any_mut(&mut self) -> &mut dyn Any;

    async fn add_component(&mut self, entity: Entity, component: Box<dyn Any + Send>);

    async fn remove_component(&mut self, entity: Entity);

    async fn process_pending(&mut self);
}

pub struct ComponentManagerImpl<C: Component> {
    storage: C::Storage,
    add_channel: (Sender<(Entity, C)>, Receiver<(Entity, C)>),
    remove_channel: (Sender<Entity>, Receiver<Entity>),
}

impl<C: Component> ComponentManagerImpl<C> {
    pub fn new() -> Self {
        let (add_tx, add_rx) = tokio::sync::mpsc::channel(1024);
        let (remove_tx, remove_rx) = tokio::sync::mpsc::channel(1024);
        Self {
            storage: C::Storage::default(),
            add_channel: (add_tx, add_rx),
            remove_channel: (remove_tx, remove_rx),
        }
    }

    pub const fn get_component_type_id_const() -> TypeId {
        TypeId::of::<C>()
    }

    pub fn get_components(&self) -> impl Iterator<Item = (Entity, &C)> {
        self.storage.iter()
    }

    pub fn get_components_mut(&mut self) -> impl Iterator<Item = (Entity, &mut C)> {
        self.storage.iter_mut()
    }
}

impl<C: Component> Default for ComponentManagerImpl<C> {
    fn default() -> Self {
        Self::new()
    }
}

#[async_trait]
impl<C: Component> ComponentManager for ComponentManagerImpl<C> {
    fn get_component_type_id(&self) -> TypeId {
        TypeId::of::<C>()
    }

    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    async fn add_component(&mut self, entity: Entity, component: Box<dyn Any + Send>) {
        if let Ok(component_c) = component.downcast::<C>() {
            self.add_channel
                .0
                .send((entity, *component_c))
                .await
                .expect("Channel unexpectedly closed");
        } else {
            unreachable!()
        }
    }

    async fn remove_component(&mut self, entity: Entity) {
        self.remove_channel
            .0
            .send(entity)
            .await
            .expect("Channel unexpectedly closed");
    }

    async fn process_pending(&mut self) {
        while let Ok((entity, component)) = self.add_channel.1.try_recv() {
            self.storage.insert(entity, component);
        }
        while let Ok(entity) = self.remove_channel.1.try_recv() {
            self.storage.remove(entity);
        }
    }
}

pub trait ComponentStorage<C: Component>: Default + Send {
    fn insert(&mut self, entity: Entity, component: C) -> Option<C>;
    fn remove(&mut self, entity: Entity) -> Option<C>;

    fn get<'a>(&'a self, entity: Entity) -> impl Iterator<Item = &'a C> + 'a
    where
        C: 'a;

    fn get_mut<'a>(&'a mut self, entity: Entity) -> impl Iterator<Item = &'a mut C> + 'a
    where
        C: 'a;

    fn iter<'a>(&'a self) -> impl Iterator<Item = (Entity, &'a C)> + 'a
    where
        C: 'a;

    fn iter_mut<'a>(&'a mut self) -> impl Iterator<Item = (Entity, &'a mut C)> + 'a
    where
        C: 'a;
}

// 稀疏的组件存储
pub struct SparseComponentStorage<C: Component> {
    storage: HashMap<Entity, Vec<C>>,
}

impl<C: Component> SparseComponentStorage<C> {
    pub fn new() -> Self {
        Self {
            storage: HashMap::new(),
        }
    }
}

impl<C: Component> Default for SparseComponentStorage<C> {
    fn default() -> Self {
        Self::new()
    }
}

impl<C: Component> ComponentStorage<C> for SparseComponentStorage<C> {
    fn insert(&mut self, entity: Entity, component: C) -> Option<C> {
        let components = self.storage.entry(entity).or_default();
        if C::EXCLUSIVE && !components.is_empty() {
            Some(component)
        } else {
            components.push(component);
            None
        }
    }

    fn remove(&mut self, entity: Entity) -> Option<C> {
        self.storage
            .get_mut(&entity)
            .and_then(|components| components.pop())
    }

    fn get<'a>(&'a self, entity: Entity) -> impl Iterator<Item = &'a C> + 'a
    where
        C: 'a,
    {
        if let Some(components) = self.storage.get(&entity) {
            components.iter()
        } else {
            [].iter()
        }
    }

    fn get_mut<'a>(&'a mut self, entity: Entity) -> impl Iterator<Item = &'a mut C>
    where
        C: 'a,
    {
        if let Some(components) = self.storage.get_mut(&entity) {
            components.iter_mut()
        } else {
            [].iter_mut()
        }
    }

    fn iter<'a>(&'a self) -> impl Iterator<Item = (Entity, &'a C)> + 'a
    where
        C: 'a,
    {
        self.storage.iter().flat_map(|(entity, components)| {
            components
                .iter()
                .map(|component| (*entity, component))
                .collect::<Vec<_>>()
        })
    }

    fn iter_mut<'a>(&'a mut self) -> impl Iterator<Item = (Entity, &'a mut C)> + 'a
    where
        C: 'a,
    {
        self.storage.iter_mut().flat_map(|(entity, components)| {
            components
                .iter_mut()
                .map(|component| (*entity, component))
                .collect::<Vec<_>>()
        })
    }
}

// 连续的组件存储
pub struct ContinuousComponentStorage<C: Component> {
    storage: Vec<Option<C>>,
    map: HashMap<Entity, HashSet<usize>>,
    map_rev: HashMap<usize, Entity>,
    free_list: Vec<usize>, // 空槽索引栈
}

impl<C: Component> Default for ContinuousComponentStorage<C> {
    fn default() -> Self {
        Self::new()
    }
}

impl<C: Component> ContinuousComponentStorage<C> {
    const INITIAL_CAPACITY: usize = 64;

    pub fn new() -> Self {
        let mut res = Self {
            storage: Vec::with_capacity(Self::INITIAL_CAPACITY),
            map: HashMap::new(),
            map_rev: HashMap::new(),
            free_list: Vec::with_capacity(Self::INITIAL_CAPACITY),
        };
        res.storage.resize_with(Self::INITIAL_CAPACITY, || None);
        res.free_list.extend((0..Self::INITIAL_CAPACITY).rev());
        res
    }
}

impl<C: Component> ComponentStorage<C> for ContinuousComponentStorage<C> {
    fn insert(&mut self, entity: Entity, component: C) -> Option<C> {
        if C::EXCLUSIVE && self.map.get(&entity).is_some_and(|idxs| !idxs.is_empty()) {
            Some(component)
        } else {
            if self.free_list.is_empty() {
                let old_len = self.storage.len();
                self.storage.extend((0..old_len).map(|_| None));
                self.free_list.extend((old_len..self.storage.len()).rev());
            }

            let idx = self.free_list.pop().unwrap();

            debug_assert!(self.storage[idx].is_none());
            self.storage[idx] = Some(component);

            if let Some(idxs) = self.map.get_mut(&entity) {
                idxs.insert(idx);
            } else {
                self.map.insert(entity, [idx].into_iter().collect());
            }
            self.map_rev.insert(idx, entity);
            None
        }
    }

    fn remove(&mut self, entity: Entity) -> Option<C> {
        let (idx, remove_entity_entry) = {
            let idxs = self.map.get_mut(&entity)?;
            let idx = idxs.iter().next().copied()?;
            idxs.remove(&idx);
            (idx, idxs.is_empty())
        };

        if remove_entity_entry {
            self.map.remove(&entity);
        }

        let component = self.storage[idx].take();
        if component.is_some() {
            self.free_list.push(idx);
        }

        self.map_rev.remove(&idx);
        component
    }

    fn get<'a>(&'a self, entity: Entity) -> impl Iterator<Item = &'a C> + 'a
    where
        C: 'a,
    {
        if let Some(idxs) = self.map.get(&entity) {
            Either::Left(idxs.iter().filter_map(|&idx| self.storage[idx].as_ref()))
        } else {
            Either::Right(std::iter::empty())
        }
    }

    fn get_mut<'a>(&'a mut self, entity: Entity) -> impl Iterator<Item = &'a mut C> + 'a
    where
        C: 'a,
    {
        if let Some(idxs) = self.map.get(&entity) {
            let mut idxs = idxs.iter().copied().collect::<Vec<_>>();
            idxs.sort_unstable();

            let mut base = 0usize;
            let mut slice = self.storage.as_mut_slice();
            let mut components = Vec::with_capacity(idxs.len());

            for idx in idxs {
                let relative_idx = idx - base;
                let (_, right) = slice.split_at_mut(relative_idx);
                let Some((slot, rest)) = right.split_first_mut() else {
                    break;
                };

                if let Some(component) = slot.as_mut() {
                    components.push(component);
                }

                slice = rest;
                base = idx + 1;
            }

            Either::Left(components.into_iter())
        } else {
            Either::Right(std::iter::empty())
        }
    }

    fn iter<'a>(&'a self) -> impl Iterator<Item = (Entity, &'a C)> + 'a
    where
        C: 'a,
    {
        self.storage.iter().enumerate().filter_map(|(i, slot)| {
            slot.as_ref()
                .map(|component| (self.map_rev.get(&i).cloned().unwrap(), component))
        })
    }

    fn iter_mut<'a>(&'a mut self) -> impl Iterator<Item = (Entity, &'a mut C)> + 'a
    where
        C: 'a,
    {
        self.storage.iter_mut().enumerate().filter_map(|(i, slot)| {
            slot.as_mut()
                .map(|component| (self.map_rev.get(&i).cloned().unwrap(), component))
        })
    }
}

#[cfg(test)]
mod tests {
    use std::any::TypeId;

    use super::{
        Component, ComponentExt, ComponentManagerImpl, ContinuousComponentStorage,
        SparseComponentStorage,
    };
    use crate::{ecs::entity::Entity, macros::component};

    #[component(storage = SparseComponentStorage, exclusive = false)]
    struct BasicComponent {
        entity: Entity,
        value: u32,
    }

    impl Default for BasicComponent {
        fn default() -> Self {
            Self {
                entity: Entity::default(),
                value: 0,
            }
        }
    }

    impl ComponentExt for BasicComponent {}

    #[component(exclusive = true, storage = ContinuousComponentStorage)]
    struct ExclusiveComponent {
        entity: Entity,
    }

    impl Default for ExclusiveComponent {
        fn default() -> Self {
            Self {
                entity: Entity::default(),
            }
        }
    }

    impl ComponentExt for ExclusiveComponent {}

    #[test]
    fn component_macro_generates_component_impl() {
        let entity = unsafe { std::mem::zeroed::<Entity>() };
        let mut component = BasicComponent { entity, value: 7 };

        assert_eq!(component.type_id(), TypeId::of::<BasicComponent>());
        assert_eq!(component.type_name(), "BasicComponent");
        assert!(!<BasicComponent as Component>::EXCLUSIVE);
        assert!(component.entity() == entity);
        assert_eq!(component.downcast_unchecked::<BasicComponent>().value, 7);

        component.downcast_unchecked_mut::<BasicComponent>().value = 11;
        assert_eq!(component.value, 11);
    }

    #[test]
    fn component_macro_supports_unordered_args() {
        assert!(<ExclusiveComponent as Component>::EXCLUSIVE);
        let _manager = ComponentManagerImpl::<ExclusiveComponent>::new();
    }
}
