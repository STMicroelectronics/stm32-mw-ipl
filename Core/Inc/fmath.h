/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Fast approximate math functions.
 */
#ifndef __FMATH_H__
#define __FMATH_H__
#include <stdint.h>
#include "common.h" /* STM32IPL added */

/* STM32IPL removed as the definitions of such functions has been added below.
float ALWAYS_INLINE fast_sqrtf(float x);
int ALWAYS_INLINE fast_floorf(float x);
int ALWAYS_INLINE fast_ceilf(float x);
int ALWAYS_INLINE fast_roundf(float x);
*/
float fast_atanf(float x);
float fast_atan2f(float y, float x);
float fast_expf(float x);
float fast_cbrtf(float d);
/* STM32IPL removed as the definitions of such functions has been added below.
float fast_fabsf(float d);
*/
float fast_log(float x);
float fast_log2(float x);
float fast_powf(float a, float b);
extern const float cos_table[360];
extern const float sin_table[360];


/* STM32IPL following functions have been added to allow their "visibility" as they are inline. */
float ALWAYS_INLINE fast_sqrtf(float x)
{
    asm volatile (
            "vsqrt.f32  %[r], %[x]\n"
            : [r] "=t" (x)
            : [x] "t"  (x));
    return x;
}

int ALWAYS_INLINE fast_floorf(float x)
{
    int i;
    asm volatile (
            "vcvt.S32.f32  %[r], %[x]\n"
            : [r] "=t" (i)
            : [x] "t"  (x));
    return i;
}

int ALWAYS_INLINE fast_ceilf(float x)
{
    int i;
    x += 0.9999f;
    asm volatile (
            "vcvt.S32.f32  %[r], %[x]\n"
            : [r] "=t" (i)
            : [x] "t"  (x));
    return i;
}

int ALWAYS_INLINE fast_roundf(float x)
{
    int i;
    asm volatile (
            "vcvtr.s32.f32  %[r], %[x]\n"
            : [r] "=t" (i)
            : [x] "t"  (x));
    return i;
}

float ALWAYS_INLINE fast_fabsf(float x)
{
    asm volatile (
            "vabs.f32  %[r], %[x]\n"
            : [r] "=t" (x)
            : [x] "t"  (x));
    return x;
}


#endif // __FMATH_H__
