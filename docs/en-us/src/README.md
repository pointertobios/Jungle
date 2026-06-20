# Jungle Engine

Jungle is an experimental game engine built on C++26 reflection, exploring the application of modern C++ standards in game development.

## Design Philosophy

- **Zero-overhead abstraction**: Compile-time reflection and template metaprogramming replace runtime type systems, with no dependency on RTTI or exceptions
- **Type safety**: Errors caught at compile time through concepts, contracts, and strong type aliases
- **Modular**: Base library, ECS core, rendering, networking, etc., layered by dependency

## Build Requirements

- GCC only (for now)
- C++26 standard
- Reflection (`-freflection`)
- Contracts (`-fcontracts`)
- RTTI disabled (`-fno-rtti`)
- Exceptions disabled (`-fno-exceptions`)
