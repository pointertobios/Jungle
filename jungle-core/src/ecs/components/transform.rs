use glam::Vec3A;
use jungle_macros::component;
use serde::{Deserialize, Serialize, Serializer};

use crate::ecs::{
    component::{ComponentExt, ContinuousComponentStorage},
    components::{ComponentDeserializeField, ComponentSerdeField, ComponentSerializeField},
    entity::Entity,
};

impl ComponentSerializeField for Vec3A {
    fn serialize_field<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        (self.x, self.y, self.z).serialize(serializer)
    }
}

impl<'de> ComponentDeserializeField<'de> for Vec3A {
    fn deserialize_field<D>(deserializer: D) -> Result<Self, D::Error>
    where
        Self: Sized,
        D: serde::Deserializer<'de>,
    {
        let (x, y, z) = <(f32, f32, f32)>::deserialize(deserializer)?;
        Ok(Vec3A::new(x, y, z))
    }
}

impl ComponentSerdeField for Vec3A {
    fn from_jaml(jaml: &str) -> Option<Self>
    where
        Self: Sized,
    {
        let parts: Vec<&str> = jaml.split_whitespace().collect();
        if parts.len() != 3 {
            return None;
        }
        let x = parts[0].parse::<f32>().ok()?;
        let y = parts[1].parse::<f32>().ok()?;
        let z = parts[2].parse::<f32>().ok()?;
        Some(Vec3A::new(x, y, z))
    }

    fn to_jaml(&self) -> String {
        format!("{} {} {}", self.x, self.y, self.z)
    }
}

#[component(storage = ContinuousComponentStorage, exclusive = true)]
#[derive(Debug, Default, PartialEq)]
pub struct Transform {
    entity: Entity,

    #[serde]
    position: Vec3A,
    #[serde]
    rotation: Vec3A,
    #[serde]
    scale: Vec3A,
}

impl ComponentExt for Transform {}
