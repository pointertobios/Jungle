use std::{
    any::{Any, TypeId},
    collections::{HashMap, HashSet},
    marker::PhantomData,
};

use anyhow::{Result, anyhow};
use async_trait::async_trait;
use blake3::hash;
use bytes::Bytes;
use itertools::Either;
use serde::{Deserialize, Serialize};
use tokio::sync::mpsc::{Receiver, Sender};

use crate::{asset::entity_tree::JamlComponent, ecs::entity::Entity};

pub type ComponentId = u128; // 组件类型 ID，使用组件类型名称的 blake3 哈希值的前 16 字节

pub trait Component: 'static + Default + Sized + Send + Sync + ComponentExt {
    type Storage: ComponentStorage<Self>;

    const EXCLUSIVE: bool; // 互斥，每个 Entity 最多只能有一个该 Component 类型的组件

    fn id() -> ComponentId {
        let digest = hash(Self::type_name().as_bytes());
        let mut bytes = [0_u8; 16];
        bytes.copy_from_slice(&digest.as_bytes()[..16]);
        ComponentId::from_le_bytes(bytes)
    }

    fn type_name() -> &'static str;

    fn from_bytes(b: Bytes) -> bincode::Result<Self>
    where
        Self: Sized + for<'de> Deserialize<'de>,
    {
        bincode::deserialize(&b)
    }

    fn to_bytes(&self) -> bincode::Result<Bytes>
    where
        Self: Sized + Serialize,
    {
        bincode::serialize(self).map(Bytes::from)
    }

    fn from_jaml(_jaml: JamlComponent) -> Option<Self> {
        None
    }

    fn to_jaml(&self) -> Option<JamlComponent> {
        None
    }

    fn as_any(&self) -> &dyn Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn downcast_unchecked<T: Component>(&self) -> &T;
    fn downcast_unchecked_mut<T: Component>(&mut self) -> &mut T;

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
    fn get_component_id(&self) -> ComponentId;
    fn as_any(&self) -> &dyn Any;
    fn as_any_mut(&mut self) -> &mut dyn Any;

    async fn add_component(&mut self, entity: Entity, component: Box<dyn Any + Send>);

    async fn remove_component(&mut self, entity: Entity);

    async fn process_pending(&mut self);

    fn get_components(&self) -> Box<dyn Iterator<Item = (Entity, &dyn Any)> + '_>;
    fn get_components_mut(&mut self) -> Box<dyn Iterator<Item = (Entity, &mut dyn Any)> + '_>;
}

pub trait ComponentSerdeHelper {
    fn build_component_from_bytes(
        &self,
        _bytes: Bytes,
    ) -> Option<bincode::Result<Box<dyn Any + Send>>> {
        None
    }

    fn build_component_data_from_jaml(&self, _jaml: JamlComponent) -> Option<Result<Bytes>> {
        None
    }

    fn build_jaml_from_component_data(&self, _bytes: &Bytes) -> Option<Result<JamlComponent>> {
        None
    }
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

pub struct ComponentSerdeHelperImpl<C: Component>(pub PhantomData<C>);

impl<C: Component + Serialize + for<'de> Deserialize<'de>> ComponentSerdeHelper
    for ComponentSerdeHelperImpl<C>
{
    fn build_component_from_bytes(
        &self,
        bytes: Bytes,
    ) -> Option<bincode::Result<Box<dyn Any + Send>>> {
        Some(C::from_bytes(bytes).map(|component| Box::new(component) as Box<dyn Any + Send>))
    }

    fn build_component_data_from_jaml(&self, jaml: JamlComponent) -> Option<Result<Bytes>> {
        let component = C::from_jaml(jaml)?;
        Some(
            bincode::serialize(&component)
                .map(Bytes::from)
                .map_err(anyhow::Error::from),
        )
    }

    fn build_jaml_from_component_data(&self, bytes: &Bytes) -> Option<Result<JamlComponent>> {
        Some(
            bincode::deserialize::<C>(bytes)
                .map_err(anyhow::Error::from)
                .and_then(|component| {
                    component
                        .to_jaml()
                        .ok_or_else(|| anyhow!("Component does not support JAML serialization"))
                }),
        )
    }
}

#[async_trait]
impl<C: Component> ComponentManager for ComponentManagerImpl<C> {
    fn get_component_id(&self) -> u128 {
        C::id()
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

    fn get_components(&self) -> Box<dyn Iterator<Item = (Entity, &dyn Any)> + '_> {
        Box::new(
            self.storage
                .iter()
                .map(|(entity, component)| (entity, component as &dyn Any)),
        )
    }

    fn get_components_mut(&mut self) -> Box<dyn Iterator<Item = (Entity, &mut dyn Any)> + '_> {
        Box::new(
            self.storage
                .iter_mut()
                .map(|(entity, component)| (entity, component as &mut dyn Any)),
        )
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
    use std::fmt;

    use serde::{
        Deserialize, Serialize,
        ser::{Impossible, SerializeStruct, Serializer},
    };

    use super::{
        Component, ComponentExt, ComponentManagerImpl, ContinuousComponentStorage,
        SparseComponentStorage,
    };
    use crate::{
        ecs::{
            components::{ComponentDeserializeField, ComponentSerdeField, ComponentSerializeField},
            entity::Entity,
        },
        macros::component,
    };

    impl ComponentSerializeField for String {
        fn serialize_field<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
        {
            self.serialize(serializer)
        }
    }

    impl<'de> ComponentDeserializeField<'de> for String {
        fn deserialize_field<D>(deserializer: D) -> Result<Self, D::Error>
        where
            Self: Sized,
            D: serde::Deserializer<'de>,
        {
            String::deserialize(deserializer)
        }
    }

    impl ComponentSerdeField for String {
        fn from_jaml(jaml: &str) -> Option<Self> {
            Some(jaml.to_string())
        }

        fn to_jaml(&self) -> String {
            self.clone()
        }
    }

    #[derive(Debug, Default, PartialEq, Eq)]
    struct CounterValue(u32);

    impl ComponentSerializeField for CounterValue {
        fn serialize_field<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
        {
            self.0.serialize(serializer)
        }
    }

    impl<'de> ComponentDeserializeField<'de> for CounterValue {
        fn deserialize_field<D>(deserializer: D) -> Result<Self, D::Error>
        where
            Self: Sized,
            D: serde::Deserializer<'de>,
        {
            Ok(Self(u32::deserialize(deserializer)?))
        }
    }

    impl ComponentSerdeField for CounterValue {
        fn from_jaml(jaml: &str) -> Option<Self> {
            Some(Self(jaml.parse::<u32>().ok()?))
        }

        fn to_jaml(&self) -> String {
            self.0.to_string()
        }
    }

    #[derive(Debug, PartialEq)]
    enum SerializedFieldValue {
        U32(u32),
        String(String),
    }

    #[derive(Debug, PartialEq)]
    struct SerializedStruct {
        name: &'static str,
        fields: Vec<(&'static str, SerializedFieldValue)>,
    }

    #[derive(Debug)]
    struct TestSerializerError(String);

    impl fmt::Display for TestSerializerError {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.write_str(&self.0)
        }
    }

    impl std::error::Error for TestSerializerError {}

    impl serde::ser::Error for TestSerializerError {
        fn custom<T: fmt::Display>(msg: T) -> Self {
            Self(msg.to_string())
        }
    }

    struct FieldValueSerializer;

    impl Serializer for FieldValueSerializer {
        type Ok = SerializedFieldValue;
        type Error = TestSerializerError;
        type SerializeSeq = Impossible<SerializedFieldValue, TestSerializerError>;
        type SerializeTuple = Impossible<SerializedFieldValue, TestSerializerError>;
        type SerializeTupleStruct = Impossible<SerializedFieldValue, TestSerializerError>;
        type SerializeTupleVariant = Impossible<SerializedFieldValue, TestSerializerError>;
        type SerializeMap = Impossible<SerializedFieldValue, TestSerializerError>;
        type SerializeStruct = Impossible<SerializedFieldValue, TestSerializerError>;
        type SerializeStructVariant = Impossible<SerializedFieldValue, TestSerializerError>;

        fn serialize_u32(self, value: u32) -> Result<Self::Ok, Self::Error> {
            Ok(SerializedFieldValue::U32(value))
        }

        fn serialize_str(self, value: &str) -> Result<Self::Ok, Self::Error> {
            Ok(SerializedFieldValue::String(value.to_string()))
        }

        fn serialize_bool(self, _value: bool) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: bool".to_string(),
            ))
        }

        fn serialize_i8(self, _value: i8) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: i8".to_string(),
            ))
        }

        fn serialize_i16(self, _value: i16) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: i16".to_string(),
            ))
        }

        fn serialize_i32(self, _value: i32) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: i32".to_string(),
            ))
        }

        fn serialize_i64(self, _value: i64) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: i64".to_string(),
            ))
        }

        fn serialize_i128(self, _value: i128) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: i128".to_string(),
            ))
        }

        fn serialize_u8(self, _value: u8) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: u8".to_string(),
            ))
        }

        fn serialize_u16(self, _value: u16) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: u16".to_string(),
            ))
        }

        fn serialize_u64(self, _value: u64) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: u64".to_string(),
            ))
        }

        fn serialize_f32(self, _value: f32) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: f32".to_string(),
            ))
        }

        fn serialize_f64(self, _value: f64) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: f64".to_string(),
            ))
        }

        fn serialize_char(self, _value: char) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: char".to_string(),
            ))
        }

        fn serialize_bytes(self, _value: &[u8]) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: bytes".to_string(),
            ))
        }

        fn serialize_none(self) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: none".to_string(),
            ))
        }

        fn serialize_some<T>(self, _value: &T) -> Result<Self::Ok, Self::Error>
        where
            T: ?Sized + Serialize,
        {
            Err(TestSerializerError(
                "unsupported test field type: some".to_string(),
            ))
        }

        fn serialize_unit(self) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: unit".to_string(),
            ))
        }

        fn serialize_unit_struct(self, _name: &'static str) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: unit struct".to_string(),
            ))
        }

        fn serialize_unit_variant(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
        ) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: unit variant".to_string(),
            ))
        }

        fn serialize_newtype_struct<T>(
            self,
            _name: &'static str,
            _value: &T,
        ) -> Result<Self::Ok, Self::Error>
        where
            T: ?Sized + Serialize,
        {
            Err(TestSerializerError(
                "unsupported test field type: newtype struct".to_string(),
            ))
        }

        fn serialize_newtype_variant<T>(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
            _value: &T,
        ) -> Result<Self::Ok, Self::Error>
        where
            T: ?Sized + Serialize,
        {
            Err(TestSerializerError(
                "unsupported test field type: newtype variant".to_string(),
            ))
        }

        fn serialize_seq(self, _len: Option<usize>) -> Result<Self::SerializeSeq, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: seq".to_string(),
            ))
        }

        fn serialize_tuple(self, _len: usize) -> Result<Self::SerializeTuple, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: tuple".to_string(),
            ))
        }

        fn serialize_tuple_struct(
            self,
            _name: &'static str,
            _len: usize,
        ) -> Result<Self::SerializeTupleStruct, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: tuple struct".to_string(),
            ))
        }

        fn serialize_tuple_variant(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
            _len: usize,
        ) -> Result<Self::SerializeTupleVariant, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: tuple variant".to_string(),
            ))
        }

        fn serialize_map(self, _len: Option<usize>) -> Result<Self::SerializeMap, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: map".to_string(),
            ))
        }

        fn serialize_struct(
            self,
            _name: &'static str,
            _len: usize,
        ) -> Result<Self::SerializeStruct, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: nested struct".to_string(),
            ))
        }

        fn serialize_struct_variant(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
            _len: usize,
        ) -> Result<Self::SerializeStructVariant, Self::Error> {
            Err(TestSerializerError(
                "unsupported test field type: struct variant".to_string(),
            ))
        }
    }

    struct StructCollector {
        name: &'static str,
        fields: Vec<(&'static str, SerializedFieldValue)>,
    }

    impl SerializeStruct for StructCollector {
        type Ok = SerializedStruct;
        type Error = TestSerializerError;

        fn serialize_field<T>(&mut self, key: &'static str, value: &T) -> Result<(), Self::Error>
        where
            T: ?Sized + Serialize,
        {
            self.fields
                .push((key, value.serialize(FieldValueSerializer)?));
            Ok(())
        }

        fn end(self) -> Result<Self::Ok, Self::Error> {
            Ok(SerializedStruct {
                name: self.name,
                fields: self.fields,
            })
        }
    }

    struct StructCaptureSerializer;

    impl Serializer for StructCaptureSerializer {
        type Ok = SerializedStruct;
        type Error = TestSerializerError;
        type SerializeSeq = Impossible<SerializedStruct, TestSerializerError>;
        type SerializeTuple = Impossible<SerializedStruct, TestSerializerError>;
        type SerializeTupleStruct = Impossible<SerializedStruct, TestSerializerError>;
        type SerializeTupleVariant = Impossible<SerializedStruct, TestSerializerError>;
        type SerializeMap = Impossible<SerializedStruct, TestSerializerError>;
        type SerializeStruct = StructCollector;
        type SerializeStructVariant = Impossible<SerializedStruct, TestSerializerError>;

        fn serialize_struct(
            self,
            name: &'static str,
            len: usize,
        ) -> Result<Self::SerializeStruct, Self::Error> {
            Ok(StructCollector {
                name,
                fields: Vec::with_capacity(len),
            })
        }

        fn serialize_bool(self, _value: bool) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_i8(self, _value: i8) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_i16(self, _value: i16) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_i32(self, _value: i32) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_i64(self, _value: i64) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_i128(self, _value: i128) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_u8(self, _value: u8) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_u16(self, _value: u16) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_u32(self, _value: u32) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_u64(self, _value: u64) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_u128(self, _value: u128) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_f32(self, _value: f32) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_f64(self, _value: f64) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_char(self, _value: char) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_str(self, _value: &str) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_bytes(self, _value: &[u8]) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_none(self) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_some<T>(self, _value: &T) -> Result<Self::Ok, Self::Error>
        where
            T: ?Sized + Serialize,
        {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_unit(self) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_unit_struct(self, _name: &'static str) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_unit_variant(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
        ) -> Result<Self::Ok, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_newtype_struct<T>(
            self,
            _name: &'static str,
            _value: &T,
        ) -> Result<Self::Ok, Self::Error>
        where
            T: ?Sized + Serialize,
        {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_newtype_variant<T>(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
            _value: &T,
        ) -> Result<Self::Ok, Self::Error>
        where
            T: ?Sized + Serialize,
        {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_seq(self, _len: Option<usize>) -> Result<Self::SerializeSeq, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_tuple(self, _len: usize) -> Result<Self::SerializeTuple, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_tuple_struct(
            self,
            _name: &'static str,
            _len: usize,
        ) -> Result<Self::SerializeTupleStruct, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_tuple_variant(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
            _len: usize,
        ) -> Result<Self::SerializeTupleVariant, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_map(self, _len: Option<usize>) -> Result<Self::SerializeMap, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }

        fn serialize_struct_variant(
            self,
            _name: &'static str,
            _variant_index: u32,
            _variant: &'static str,
            _len: usize,
        ) -> Result<Self::SerializeStructVariant, Self::Error> {
            Err(TestSerializerError(
                "expected struct serialization".to_string(),
            ))
        }
    }

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

    #[component(storage = SparseComponentStorage, exclusive = false)]
    struct OtherComponent {
        entity: Entity,
    }

    impl Default for OtherComponent {
        fn default() -> Self {
            Self {
                entity: Entity::default(),
            }
        }
    }

    impl ComponentExt for OtherComponent {}

    #[component(storage = SparseComponentStorage, exclusive = true)]
    #[derive(Debug)]
    struct SerializableComponent {
        entity: Entity,
        #[serde]
        name: String,
        #[serde]
        counter: CounterValue,
        cache: u32,
    }

    impl PartialEq for SerializableComponent {
        fn eq(&self, other: &Self) -> bool {
            self.entity == other.entity
                && self.name == other.name
                && self.cache == other.cache
                && self.counter == other.counter
        }
    }

    impl Default for SerializableComponent {
        fn default() -> Self {
            Self {
                entity: Entity::default(),
                name: String::new(),
                counter: CounterValue::default(),
                cache: 99,
            }
        }
    }

    impl ComponentExt for SerializableComponent {}

    #[test]
    fn component_macro_generates_component_impl() {
        let entity = unsafe { std::mem::zeroed::<Entity>() };
        let mut component = BasicComponent { entity, value: 7 };
        let digest = blake3::hash("BasicComponent".as_bytes());
        let mut bytes = [0_u8; 16];
        bytes.copy_from_slice(&digest.as_bytes()[..16]);

        assert_eq!(BasicComponent::type_name(), "BasicComponent");
        assert_eq!(BasicComponent::id(), u128::from_le_bytes(bytes));
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

    #[test]
    #[should_panic(expected = "component downcast failed")]
    fn component_macro_panics_on_wrong_downcast_type() {
        let component = BasicComponent {
            entity: Entity::default(),
            value: 7,
        };

        let _ = component.downcast_unchecked::<OtherComponent>();
    }

    #[test]
    #[should_panic(expected = "component downcast failed")]
    fn component_macro_panics_on_wrong_mut_downcast_type() {
        let mut component = BasicComponent {
            entity: Entity::default(),
            value: 7,
        };

        let _ = component.downcast_unchecked_mut::<OtherComponent>();
    }

    #[test]
    fn component_macro_serializes_only_marked_fields() {
        let component = SerializableComponent {
            entity: Entity::default(),
            name: "player".to_string(),
            counter: CounterValue(7),
            cache: 99,
        };

        let serialized = component
            .serialize(StructCaptureSerializer)
            .expect("component serialization should succeed");

        assert_eq!(
            serialized,
            SerializedStruct {
                name: "SerializableComponent",
                fields: vec![
                    ("name", SerializedFieldValue::String("player".to_string()),),
                    ("counter", SerializedFieldValue::U32(7)),
                ],
            }
        );

        let bytes = bincode::serialize(&component).expect("bincode serialization should succeed");
        let restored: SerializableComponent =
            bincode::deserialize(&bytes).expect("bincode deserialization should succeed");

        assert_eq!(restored, component);
    }

    #[test]
    fn component_macro_converts_marked_fields_to_and_from_jaml() {
        let component = SerializableComponent {
            entity: Entity::default(),
            name: "player".to_string(),
            counter: CounterValue(7),
            cache: 123,
        };

        let jaml = component
            .to_jaml()
            .expect("component jaml serialization should succeed");

        assert_eq!(jaml.get("name"), Some(&"player".to_string()));
        assert_eq!(jaml.get("counter"), Some(&"7".to_string()));
        assert_eq!(jaml.len(), 2);

        let restored = SerializableComponent::from_jaml(jaml)
            .expect("component jaml deserialization should succeed");

        assert_eq!(restored.name, "player");
        assert_eq!(restored.counter, CounterValue(7));
        assert_eq!(restored.cache, 99);
    }
}
