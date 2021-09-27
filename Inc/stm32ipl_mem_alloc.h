/**
 ******************************************************************************
 * @file   stm32ipl_mem_alloc.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - memory allocation wrapper header file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * Portions of this file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef __STM32IPL_MEM_ALLOC_H_
#define __STM32IPL_MEM_ALLOC_H_

#ifdef STM32IPL

#include <stdint.h>
#include "umm_malloc.h" // FIXME CMARCH: 2021.07.22 provo a riabilitarlo

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
 * Do not use at application side!
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

#endif // STM32IPL

#endif /* __STM32IPL_MEM_ALLOC_H_ */
