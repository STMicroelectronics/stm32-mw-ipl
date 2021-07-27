/**
 ******************************************************************************
 * @file   memory_alloc.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - memory allocation wrapper header file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * Portions of this file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This software component is licensed under MIT License, the "License";
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *                        opensource.org/licenses/MIT
 *
 ******************************************************************************
 */

#ifndef ___MEMORY_ALLOC_H_
#define ___MEMORY_ALLOC_H_

//#include <stdio.h>
#include <stdint.h>
#include "umm_malloc.h" // CMARCH: 2021.07.22 provo a riabilitarlo

#define FB_ALLOC_NO_HINT		0
#define FB_ALLOC_PREFER_SPEED	1
#define FB_ALLOC_PREFER_SIZE	2

void umm_alloc_fail(void);

/* General purpose allocation functions.
 * They can be used at app side.
 */
void* xalloc(uint32_t size);
void* xalloc_try_alloc(uint32_t size);
void* xalloc0(uint32_t size);
void xfree(void *mem);
void* xrealloc(void *mem, uint32_t size);

/* Frame buffer allocation functions.
 * They are for openmv internals only.
 * Do not use at app side!
 */
void fb_init(void);
void fb_alloc_fail(void);
uint32_t fb_avail(void);
void fb_alloc_mark(void);
void fb_alloc_free_till_mark(void);
void* fb_alloc(uint32_t size, int hints);
void* fb_alloc0(uint32_t size, int hints);
void* fb_alloc_all(uint32_t *size, int hints);
void* fb_alloc0_all(uint32_t *size, int hints);
void fb_free(void);
void fb_free_all(void);


#endif /* ___MEMORY_ALLOC_H_ */
