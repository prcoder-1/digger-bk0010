#ifndef STDINT_H
#define STDINT_H

typedef signed char int8_t;          // 8-bit signed integer
typedef unsigned char uint8_t;       // 8-bit unsigned integer
typedef short int int16_t;           // 16-bit signed integer
typedef unsigned short int uint16_t; // 16-bit unsigned integer
typedef int int32_t;                 // 32-bit signed integer
typedef unsigned int uint32_t;       // 32-bit unsigned integer

// Limits for int8_t
#define INT8_MIN   (-128)
#define INT8_MAX   (127)
#define UINT8_MAX  (255)

// Limits for int16_t
#define INT16_MIN  (-32768)
#define INT16_MAX  (32767)
#define UINT16_MAX (65535)

// Limits for int32_t
#define INT32_MIN  (-2147483647 - 1)
#define INT32_MAX  (2147483647)
#define UINT32_MAX (4294967295U)

#endif /* STDINT_H */
