# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)
project(Jungle VERSION 0.1.0 LANGUAGES CXX)

# Compiler and linker config

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)


include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(ProcessorCount)

ProcessorCount(CPUS)

enable_testing()

# Dependencies

# Jungle config

add_subdirectory(jungle-base)
add_subdirectory(jungle-tasks)
add_subdirectory(jungle-core)
add_subdirectory(jungle-api)
add_subdirectory(jungle-client)
add_subdirectory(jungle-server)
add_subdirectory(jungle-ui)

add_subdirectory(tests)
