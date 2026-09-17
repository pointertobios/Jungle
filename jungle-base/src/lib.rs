#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct StringID {
    id: u128,
}

impl StringID {
    pub const fn of(string: &'static str) -> Self {
        StringID {
            id: const_fnv1a_hash::fnv1a_hash_str_128(string),
        }
    }
}
