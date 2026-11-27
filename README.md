# Honey

## Honey ECS

a sparse/dense set ECS implementation built around the idea of making a simple, non-threatening ECS that 1. is simple to use 2. provides core ECS functionality and 3. accomodates all games types.

### features
 - 4,294,967,295 entity limit
 - 65,535 component limit
 - low static memory overhead
 - full stderr logging system for errors & warnings
 - modular design supports multiple ECS instances
 - easy to use Makefile build system
 - full doxygen documentation
 - ISO c99, only std libs, no compiler builtins, simple c++ support 
 - more coming soon... (CMake support, multi-component querying, tag components)

## Honey Win32

a win32 windowing library with no dependencies that utilizes a simple callback type event system, created to provide a high performant vulkan frontend.

### features
 - multi-window support
 - custom cursor/icon support
 - dpi awareness w/ optional user dpi scaling (real pixel values)
 - easy to use callback based event system
 - simple main loop for MULTIPLE windows (hny_window_valid())
 - ISO c99, std libs + windows.h, no compiler builtins, simple c++ support
 - more coming soon as I work w/ vulkan
 - **NOTE**: honey_win32 registers a process-wide raw input device unconditionally.

## build
 - install [Makefile](https://www.gnu.org/software/make/)
 - 'make' or 'make build' for lib (libhoney.a) - use honey_ecs.h/honey_win32.h in include/
 - 'make docs' for documentation (open docs/index.html)
 - 'make clean' to clean up both
 
shoutout [Flecs](https://github.com/sandermertens/flecs), [EnTT](https://github.com/skypjack/entt), and [GLFW](https://github.com/glfw/glfw) for a TON of inspiration behind many of the design choices in Honey