/**
 ******************************************************************************
 * @file   stm32ipl_mem_alloc.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - memory allocation module
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

#ifdef STM32IPL

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "stm32ipl_mem_alloc.h"
#include "umm_malloc.h"

#define FB_ALLOC_MAX_ENTRY		1000

static uint32_t g_fb_alloc_stack[FB_ALLOC_MAX_ENTRY];
static uint32_t g_fb_alloc_inext = 0;
static uint32_t g_fb_alloc_imark = 0;

/* xalloc and fb_alloc are used by openmv functions.
 * STM32IPL re-implements such functions by wrapping umm functions.
 */

__attribute__((weak)) void STM32Ipl_FaultHandler(const char *error)
{
	while (1);
}

void umm_alloc_fail(void)
{
	STM32Ipl_FaultHandler("umm_alloc() failure");
}

void *xalloc(uint32_t size)
{
	return umm_malloc(size);
}

void *xalloc_try_alloc(uint32_t size)
{
	return umm_malloc(size);
}

void *xalloc0(uint32_t size)
{
    void *mem = umm_malloc(size);

    if (mem == NULL)
        return NULL;

    memset(mem, 0, size);
	
    return mem;
}

void xfree(void *mem)
{
	umm_free(mem);
}

void *xrealloc(void *mem, uint32_t size)
{
	return umm_realloc(mem, size);
}

void fb_init(void)
{
	memset(g_fb_alloc_stack, 0, sizeof(uint32_t) * FB_ALLOC_MAX_ENTRY);
	g_fb_alloc_inext = 0;
	g_fb_alloc_imark = 0;
}

void fb_alloc_fail(void)
{
	STM32Ipl_FaultHandler("fb_alloc() failure");
}

uint32_t fb_avail(void)
{
	return umm_max_free_block_size();
}

void fb_alloc_mark(void)
{
	g_fb_alloc_imark = g_fb_alloc_inext;
}

void fb_alloc_free_till_mark(void)
{
	int32_t e = g_fb_alloc_inext - g_fb_alloc_imark;

	for (int i = 0; i < e; i++)
		fb_free();
}

void *fb_alloc(uint32_t size, int hints)
{
	void *p = NULL;

	if (g_fb_alloc_inext == FB_ALLOC_MAX_ENTRY) {
		fb_alloc_fail();
		return NULL;
	}

	p = umm_malloc(size);
	if (p)
		g_fb_alloc_stack[g_fb_alloc_inext++] = (uint32_t)p;
	else
		fb_alloc_fail();

	return p;
}

void *fb_alloc0(uint32_t size, int hints)
{
	void *p = NULL;

	p = fb_alloc(size, hints);
	if (p)
		memset(p, 0, size);
	else
		fb_alloc_fail();

	return p;
}

void *fb_alloc_all(uint32_t *size, int hints)
{
	uint32_t max_size = fb_avail();
	void *p = NULL;

	p = fb_alloc(max_size, hints);
	*size = (p == NULL) ? 0 : max_size;

	return p;
}

void *fb_alloc0_all(uint32_t *size, int hints)
{
	uint32_t max_size = fb_avail();
	void * p = NULL;

	p = fb_alloc0(max_size, hints);
	*size = (p == NULL) ? 0 : max_size;

	return p;
}

void fb_free(void)
{
	if (g_fb_alloc_inext == 0)
		return;

	g_fb_alloc_inext--;
	umm_free((void*)g_fb_alloc_stack[g_fb_alloc_inext]);
	g_fb_alloc_stack[g_fb_alloc_inext] = 0;
}

void fb_free_all(void)
{
	uint32_t e = g_fb_alloc_inext;
	for (int i = 0; i < e; i++)
		fb_free();
}

#endif // STM32IPL
