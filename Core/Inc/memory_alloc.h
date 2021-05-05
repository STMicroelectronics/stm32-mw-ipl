/**
 ******************************************************************************
  * @file memory_alloc.h
  * @author  SRA AI Application Team
  * @brief memory allocation wrapper
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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



#ifndef MEMORY_ALLOC_H_
#define MEMORY_ALLOC_H_

#include <stdio.h>
#include <stdint.h>
#include "umm_malloc.h"

/*
 * frame buffer allocation
 * for openmv imlib internals do not use !!
 */
// FB ALLOC
#define FB_ALLOC_NO_HINT 0
#define FB_ALLOC_PREFER_SPEED 1
#define FB_ALLOC_PREFER_SIZE 2
void fb_alloc_fail(void);
uint32_t fb_avail(void);
void fb_alloc_mark(void);
void fb_alloc_free_till_mark(void);
void *fb_alloc(uint32_t size, int hints);
void *fb_alloc0(uint32_t size, int hints);
void *fb_alloc_all(uint32_t *size, int hints); // returns pointer and sets size
void *fb_alloc0_all(uint32_t *size, int hints); // returns pointer and sets size
void fb_free(void);
void fb_free_all(void);
void fb_init(void);


/*
 * general purpose allocation
 */
// X ALLOC
void *xalloc(uint32_t size);
void *xalloc_try_alloc(uint32_t size);
void *xalloc0(uint32_t size);
void xfree(void *mem);
void *xrealloc(void *mem, uint32_t size);

// umm_malloc
void umm_alloc_fail(void);


#endif /* MEMORY_ALLOC_H_ */
