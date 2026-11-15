a simple sparse/dense set ECS implementation.

### features:
 - 2^32 entity limit (~4 billion) + 32 generation bits -> no stale entity issues
 - low memory overhead + automatic memory allocation -> only pay for what you need
 - simple entity creation deletion + component registration w/ payload
 - smart entity recycling -> memory won't skyrocket with elongated usage
 - optimized preprocessor utilization implementation -> no extra runtime overhead
 - full doxygen documentation - run 'doxygen Doxyfile' & open index.html in build/doxygen/html
 - modular build - self-contained 'world' struct contains everything
 - portability: ISO c99, no external libs, no gcc/clang builtins, simple c++ support 
 - full stderr logging system for errors & warnings
 - more coming soon... (multi-component querying, 'tag' components)

### startup:
 - include fks.h then run fks_init() to get your ecs world. make sure to run fks_fini() once you're done to avoid memory leaks
 - compile with fks.c in your project

### changes (v2.1):
 - bugfixes/readme updates/document updates/c++ support
---
#### I recommend installing [Makefile](https://www.gnu.org/software/make/) to build & run.
#### Run 'doxygen Doxyfile' & open index.html in build/doxygen/html for documentation.