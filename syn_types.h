// =====================================
//           SAYANA'S TYPES
// =====================================
#ifndef SYN_TYPES_H
#define SYN_TYPES_H

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

// Looking at MSVC's inttype, these are the typedefs
typedef signed char i8;
typedef short       i16;
typedef int         i32;
typedef long long   i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

// Source: Microsoft's "Type float" and "C and C++ Integer Limits"
#define I8_MIN     (-128)
#define I8_MAX     127
#define U8_MIN     (0)
#define U8_MAX     0xff

#define I16_MIN    (-32768)
#define I16_MAX    32767
#define U16_MIN    (0)
#define U16_MAX    0xffff

#define I32_MIN    (-2147483647 - 1)
#define I32_MAX    2147483647
#define U32_MIN    (0)
#define U32_MAX    0xffffffff

#define I64_MIN    (-9223372036854775807i64 - 1)
#define I64_MAX    9223372036854775807i64
#define U64_MIN    (0)
#define U64_MAX    0xffffffffffffffffui64

#define F32_MIN    1.175494351e-38f
#define F32_MAX    3.402823466e+38f
#define F64_MIN    2.2250738585072014e-308
#define F64_MAX    1.7976931348623157e+308

#endif // SYN_TYPES_H
