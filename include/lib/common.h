/*
 *  Copyright (C) 2023 Nicolai Brand, Callum Gran
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/* unsigned integral types */
#define Rune i32
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define u128 uint128_t
#define u256 uint256_t

/* signed integral types */
#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t
#define i128 iint128_t
#define i256 iint256_t

/* common */
#define VA_NUMBER_OF_ARGS(...) (sizeof((int[]){ __VA_ARGS__ }) / sizeof(int))

#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) ((x) << 40)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(a, x, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) : (x))

#define BYTES_EQUAL(a, b, size, pred)            \
    do {                                         \
	size_t __size = (size);                  \
	register const unsigned char *__a = (a); \
	register const unsigned char *__b = (b); \
	do {                                     \
	    pred = (*__a++ == *__b++);           \
	} while (pred && --__size != 0);         \
    } while (0)

#ifdef DYNAMIC_MACROS
/* Dynamic Methods */

/*
    Macro that ensures that the size of a dynamic object is not exceeded.
    @param size size_t: current size of the object.
    @param cap size_t: current capacity of the object.
    @param vals void**: pointer to the object.
*/
#define ENSURE_CAP(size, cap, vals)                                \
    do {                                                           \
	if (size == cap) {                                         \
	    cap <<= 1;                                             \
	    vals = (void **)(realloc(vals, cap * sizeof(void *))); \
	}                                                          \
    } while (0)

/*
    Macro that reduces the size of a dynamic object.
    @param size size_t: current size of the object.
    @param cap size_t: current capacity of the object.
    @param vals void**: pointer to the object.
*/
#define REDUCE_CAP(size, cap, vals)                                \
    do {                                                           \
	if (size == (cap >> 1)) {                                  \
	    cap >>= 1;                                             \
	    vals = (void **)(realloc(vals, cap * sizeof(void *))); \
	}                                                          \
    } while (0)

#endif

#endif /* COMMON_H */
