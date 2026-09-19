use std::{fmt::Display, num::NonZeroU64};

use base64::{Engine, engine::general_purpose};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Entity {
    id: Option<NonZeroU64>,
}

impl Display for Entity {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = if let Some(id) = self.id {
            general_purpose::STANDARD.encode(id.get().to_le_bytes())
        } else {
            String::from("None")
        };
        f.write_str(&format!("Entity({})", s))
    }
}

impl Entity {
    pub fn from_u64(id: u64) -> Self {
        if id == 0 {
            Self { id: None }
        } else {
            Self {
                id: NonZeroU64::new(id),
            }
        }
    }
}
