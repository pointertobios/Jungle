use std::num::NonZeroU64;

use serde::{Deserialize, Serialize};

#[derive(Debug, Default, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Entity(Option<NonZeroU64>);

impl Serialize for Entity {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        if let Some(id) = self.0 {
            serializer.serialize_u64(id.get())
        } else {
            serializer.serialize_u64(0)
        }
    }
}

impl<'de> Deserialize<'de> for Entity {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let id = u64::deserialize(deserializer)?;
        Ok(Self(NonZeroU64::new(id)))
    }
}

impl Entity {
    pub fn new(id: u64) -> Self {
        Self(NonZeroU64::new(id))
    }

    pub fn is_valid(&self) -> bool {
        self.0.is_some()
    }
}
