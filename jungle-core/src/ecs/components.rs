use serde::{Deserialize, Serialize, Serializer};

pub mod node;
pub mod prefab_ref;
pub mod transform;

pub trait ComponentSerializeField {
    fn serialize_field<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer;
}

pub trait ComponentDeserializeField<'de> {
    fn deserialize_field<D>(deserializer: D) -> Result<Self, D::Error>
    where
        Self: Sized,
        D: serde::Deserializer<'de>;
}

pub trait ComponentSerdeField:
    ComponentSerializeField + for<'de> ComponentDeserializeField<'de>
{
    fn from_jaml(jaml: &str) -> Option<Self>
    where
        Self: Sized;

    fn to_jaml(&self) -> String;
}
