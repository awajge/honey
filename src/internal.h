#ifndef INTERNAL_H
#define INTERNAL_H

#include <stdint.h>

#define HNY_STATIC_ASSERT(name, expr) typedef char name[(expr) ? 1 : -1];

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t  i32;
typedef unsigned char uc;
typedef unsigned int uint;
typedef wchar_t wchar;

#endif