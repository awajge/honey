a simple sparse/dense set ECS implementation.

### features:
 - 2^32 entity limit (~4 billion) + 32 generation bits -> no stale entity issues
 - low memory overhead + automatic memory allocation -> only pay for what you need
 - simple entity creation deletion + component registration w/ payload
 - smart entity recycling -> memory won't skyrocket with elongated usage
 - optimized preprocessor utilization implementation -> no extra runtime overhead
 - full doxygen documentation - run 'make docs' & open index.html in build/doxygen/html
 - modular build - self-contained 'world' struct contains everything
 - portability: ISO c99, no external libs, no gcc/clang builtins, simple c++ support 
 - full stderr logging system for errors & warnings
 - more coming soon... (multi-component querying, 'tag' components)

### changes (v2.2):
 - new build system/documentation/.gitignore

### build:
 - 'make' | 'make all' for lib (libhoney.a)
 - 'make docs' for documentation (open docs/html/index.html)
 - 'make clean' to clean up both