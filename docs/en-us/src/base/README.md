# Jungle Base Library

`jungle-base` is the infrastructure layer of the Jungle engine, providing the type system, serialization framework, reflection tools, test framework, and other low-level capabilities. All upper-layer modules (`jungle-core`, `jungle-server`, etc.) depend on this library.

## Module Structure

```
include/jungle/
├── concepts.h        # Concept constraints (Debug, is_enum, etc.)
├── debug.h           # Reflection-based generic debug output
├── meta.h            # Reflection metaprogramming utilities
├── panic.h           # Unrecoverable error handling
├── preusing.h        # Centralized common type alias imports
├── container/        # Containers
│   ├── hash_map.h    # Custom hash map
│   └── mpsc.h        # MPSC lock-free bounded queue
├── serde/            # Serialization/deserialization framework
│   ├── serde.h       # Concept definitions and annotations
│   ├── serialize.h   # SerializeTarget base class and serialize() free function
│   └── deserialize.h # DeserializeSource base class and deserialize() free function
├── test/             # Test framework
│   └── test.h        # JUNGLE_SYNC_TEST macros and assertions
├── types/            # Fundamental types
│   ├── int.h         # Fixed-width integer aliases (u8, i32, usize, etc.)
│   └── uchar.h       # Unicode character and string (uchar, ustr)
└── util/             # Utilities
    ├── murmur.h      # MurmurHash
    ├── parse.h       # Base64 encoding view
    ├── type_id.h     # Compile-time type ID
    └── types.h       # Type trait utilities
```

## Build Requirements

- C++23 standard (enabled with `-std=c++23`)
- Reflection extension enabled (`-freflection`)
- Contracts enabled (`-fcontracts`)
- RTTI and exceptions disabled (`-fno-rtti -fno-exceptions`)

## Key Components

### Type System

`types/int.h` defines fixed-width integer aliases corresponding to `std` types:

| Alias        | Corresponding Type               |
| ------------ | -------------------------------- |
| `u8` ~ `u64` | `std::uint8_t` ~ `std::uint64_t` |
| `i8` ~ `i64` | `std::int8_t` ~ `std::int64_t`   |
| `usize`      | `std::size_t`                    |
| `isize`      | `std::ptrdiff_t`                 |

`types/uchar.h` provides `uchar` (Unicode code point) and `ustr` (UTF-8 string), with `std::format` support.

### Reflection Utilities (`meta.h`)

Compile-time type introspection based on C++26 reflection:

- `has_annotation()` — check whether a type/member has a given annotation
- `has_template_annotation()` — check whether a type/member has an instance of a given template annotation
- `nth_template_annotation_argument_of()` — retrieve the Nth argument of a template annotation
- `is_specialization_of_template()` — check whether a type is an instance of a given template
- `nonstatic_data_members_with_annotation()` — retrieve all non-static data members with a given annotation

### Serialization Framework (`serde/`)

A reflection-based, type-safe serialization/deserialization framework. See [Serde Serialization/Deserialization Framework](./serde.md).

Key features:
- Zero macros, zero external code generation — relies entirely on C++26 compile-time reflection
- CRTP design: implement `SerializeTarget` or `DeserializeSource` to integrate
- Annotations control field participation policies (`[[=customized]]` / `[[=field]]`)
- Supports field-level customizers (`[[=customize<C>]]`)
- Recursively handles nested structs, containers, and `std::optional`

### Test Framework (`test/`)

Lightweight synchronous test framework:

```cpp
JUNGLE_SYNC_TEST(my_test) {
    JUNGLE_SYNC_ASSERT(condition, "failure message {}", args...);
    JUNGLE_SYNC_SUCCESS();
}
```

- Tests are registered automatically via static initialization
- Assertion failures return structured error messages with source location
- Test results printed to stdout

### Containers

#### hash_map

A custom open-addressing hash map with Robin Hood hashing, supporting tombstone reuse and automatic growth/rehash. See `include/jungle/container/hash_map.h`.

#### MPSC Queue

A lock-free bounded multiple-producer single-consumer queue. See [MPSC Lock-Free Bounded Queue](./mpsc.md).
