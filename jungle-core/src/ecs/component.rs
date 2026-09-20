use std::{fmt::Display, num::NonZeroU64};

use base64::{Engine, engine::general_purpose};

use crate::ecs::storage::ComponentStorage;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ComponentID {
    id: Option<NonZeroU64>,
}

impl Display for ComponentID {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = if let Some(id) = self.id {
            general_purpose::STANDARD.encode(id.get().to_le_bytes())
        } else {
            String::from("None")
        };
        f.write_str(&format!("Component({})", s))
    }
}

impl ComponentID {
    pub fn from_u64(id: u64) -> Self {
        Self {
            id: NonZeroU64::new(id),
        }
    }
}

pub trait Component: Default {
    type Storage: ComponentStorage<Self>;

    fn name(&self) -> &'static str;
}
