/**
 ******************************************************************************
 * @file   draw_dewarp.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - helpers draw triangle routine
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "draw_dewarp.h"

#include <assert.h>
#include <stdlib.h>

struct point {
	int x;
	int y;
	float u_s;
	float v_s;
};

struct interpolator_ctx {
	struct point *p1;
	struct point *p2;
	struct point *p3;
	float det;
	float a_u;
	float a_v;
	float b_u;
	float b_v;
};

const float rounding = 0.5;

static float interpolate(float i0, float d0, float i1, float d1, float ix)
{
	float slope;

	assert(i1 - i0 != 0);
	slope = (d1 - d0) / (i1 - i0);

	return d0 + slope * (ix - i0);
}

static struct point point_init(int x, int y, float u_s, float v_s)
{
	struct point res = {
		.x = x,
		.y = y,
		.u_s = u_s,
		.v_s = v_s,
	};

	return res;
}

static void point_swap(struct point *p0, struct point *p1)
{
	struct point tmp = *p0;

	*p0 = *p1;
	*p1 = tmp;
}

static void interpolator_init(struct interpolator_ctx *ictx, struct point *p1, struct point *p2, struct point *p3)
{
	ictx->p1 = p1;
	ictx->p2 = p2;
	ictx->p3 = p3;

	ictx->det = (p2->y - p3->y) * (p1->x - p3->x) + (p3->x - p2->x) * (p1->y - p3->y);
	assert(ictx->det);
	ictx->a_u = (p2->y - p3->y) / ictx->det;
	ictx->a_v = (p3->y - p1->y) / ictx->det;
}

static void interpolator_update(struct interpolator_ctx *ictx, int y)
{
	struct point *p1 = ictx->p1;
	struct point *p2 = ictx->p2;
	struct point *p3 = ictx->p3;

	ictx->b_u = ((p3->x - p2->x) * (y - p3->y)) / ictx->det;
	ictx->b_v = ((p1->x - p3->x) * (y - p3->y)) / ictx->det;
}

static void draw_point(int x, int y, draw_dewarp_pel dpel, void *ctx, struct interpolator_ctx *ictx)
{
	struct point *p1 = ictx->p1;
	struct point *p2 = ictx->p2;
	struct point *p3 = ictx->p3;
	float u, v;
	float z_u, z_v;

	u = ictx->a_u * (x - p3->x) + ictx->b_u;
	v = ictx->a_v * (x - p3->x) + ictx->b_v;

	z_u = (1 - u - v) * p3->u_s + u * p1->u_s + v * p2->u_s;
	z_v = (1 - u - v) * p3->v_s + u * p1->v_s + v * p2->v_s;

	dpel(ctx, x, y, z_u, z_v);
}

static void scan_line_ok(struct point *p0, struct point *p1, draw_dewarp_pel dpel, void *ctx, struct interpolator_ctx *ictx)
{
	int x;

	interpolator_update(ictx, p0->y);
	for (x = p0->x; x <= p1->x; x++) {
		draw_point(x, p0->y, dpel, ctx, ictx);
	}
}

static void scan_line(struct point *p0, struct point *p1, draw_dewarp_pel dpel, void *ctx, struct interpolator_ctx *ictx)
{
	if (p0->x < p1->x)
		scan_line_ok(p0, p1, dpel, ctx, ictx);
	else
		scan_line_ok(p1, p0, dpel, ctx, ictx);
}

static void draw_triangle_fill_impl(struct point *p0, struct point *p1, struct point *p2, draw_dewarp_pel dpel, void *ctx)
{
	struct interpolator_ctx ictx;
	struct point ps;
	struct point pe;
	int y;

	/* order point so y0 <= y1 <= y2 */
	if (p1->y < p0->y) point_swap(p0, p1);
	if (p2->y < p0->y) point_swap(p0, p2);
	if (p2->y < p1->y) point_swap(p1, p2);

	interpolator_init(&ictx, p0, p1, p2);

	/* scan points between y0 and y1 */
	if (p0->y == p1->y) {
		scan_line(p0, p1, dpel, ctx, &ictx);
	} else {
		for (y = p0->y; y <= p1->y; y++) {
			ps.y = y;
			ps.x = (int)(interpolate(p0->y, p0->x, p1->y, p1->x, y) + rounding);
			pe.y = y;
			pe.x = (int)(interpolate(p0->y, p0->x, p2->y, p2->x, y) + rounding);
			scan_line(&ps, &pe, dpel, ctx, &ictx);
		}
	}

	/* scan points between y1 and y2 */
	if (p1->y == p2->y) {
		scan_line(p1, p2, dpel, ctx, &ictx);
	} else {
		for (y = p1->y + 1; y <= p2->y; y++) {
			ps.y = y;
			ps.x = (int)(interpolate(p1->y, p1->x, p2->y, p2->x, y) + rounding);
			pe.y = y;
			pe.x = (int)(interpolate(p0->y, p0->x, p2->y, p2->x, y) + rounding);
			scan_line(&ps, &pe, dpel, ctx, &ictx);
		}
	}
}

void draw_dewarp_triangle_fill(int x0, int y0, float u0_s, float v0_s, int x1, int y1, float u1_s, float v1_s,
							   int x2, int y2, float u2_s, float v2_s, draw_dewarp_pel dpel, void *ctx)
{
	struct point p0 = point_init(x0, y0, u0_s, v0_s);
	struct point p1 = point_init(x1, y1, u1_s, v1_s);
	struct point p2 = point_init(x2, y2, u2_s, v2_s);

	draw_triangle_fill_impl(&p0, &p1, &p2, dpel, ctx);
}
