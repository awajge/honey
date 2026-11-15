a sparse/dense set ECS implementation built around the idea of making a simple, non-threatening ECS that 1. is simple to use 2.provides core ECS functionality and 3. accomodates all games types, AAA to pong.

## features
 - 4,294,967,295 entity limit
 - 65,535 component limit
 - low static memory overhead
 - full stderr logging system for errors & warnings
 - modular design supports multiple ECS instances
 - easy to use Makefile build system
 - full doxygen documentation
 - ISO c99, only std libs, no compiler builtins, simple c++ support 
 - more coming soon... (CMake support, multi-component querying, tag components)

## changes (v1.0.0)
 - first release

## build
 - install [Makefile](https://www.gnu.org/software/make/)
 - 'make' or 'make build' for lib (libhoney.a) - use honey.h in include/
 - 'make docs' for documentation (open docs/index.html)
 - 'make clean' to clean up both
---
shoutout [Flecs](https://github.com/sandermertens/flecs) & [EnTT](https://github.com/skypjack/entt) for a TON of inspiration behind many of the design choices :)