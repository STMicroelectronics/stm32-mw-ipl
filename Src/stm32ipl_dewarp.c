/**
 ******************************************************************************
 * @file   stm32ipl_dewarp.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - dewarping module
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

#include "stm32ipl.h"

#include "stm32ipl_imlib_int.h"
#include "draw_dewarp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGE_BPP_NB (IMAGE_BPP_JPEG + 1)
#define MAPXY_TYPE_NB (MAPXY_SPARSE_FLOAT + 1)
#define DEWARP_ALGO_NB (DEWARP_BILINEAR + 1)

typedef void (*dewarp_fct)(const image_t *, image_t *, const mapxy_t *);

struct dpel_ctx {
	const image_t *isrc;
	image_t *idst;
};

static int coord_to_map_idx(int r, int c, int idx, int w, int elem_nb)
{
	return (r * w + c) * elem_nb + idx;
}

static uint8_t clamp(uint8_t v, uint8_t max)
{
	return v <= max ? v : max;
}

static uint8_t interpolate(uint8_t value[4], float dx, float dy)
{
	return (uint8_t) (dx * dy * (value[3] - value[2] + value[0] - value[1]) +
		   dx * (value[1] - value[0]) +
		   dy * (value[2] - value[0]) +
		   value[0]);
}

static void read_grayscale_quad(uint8_t *src, int xi, int yi, int w, int h, uint8_t *v)
{
	int x, y;

	x = xi < w ? xi : w - 1;
	y = yi < h ? yi : h - 1;
	*v++ = src[coord_to_map_idx(y, x, 0, w, 1)];

	x = xi + 1 < w ? xi + 1 : w - 1;
	*v++ = src[coord_to_map_idx(y, x, 0, w, 1)];

	x = xi < w ? xi : w - 1;
	y = yi + 1 < h ? yi + 1 : h - 1;
	*v++ = src[coord_to_map_idx(y, x, 0, w, 1)];

	x = xi + 1 < w ? xi + 1 : w - 1;
	*v++ = src[coord_to_map_idx(y, x, 0, w, 1)];
}

static void read_rgb565_quad(uint16_t *src, int xi, int yi, int w, int h, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint16_t p;
	int x, y;

	x = xi < w ? xi : w - 1;
	y = yi < h ? yi : h - 1;
	p = src[coord_to_map_idx(y, x, 0, w, 1)];
	*r++ = p >> 11;
	*g++ = (p >> 5) & 0x3f;
	*b++ = p & 0x1f;

	x = xi + 1 < w ? xi + 1 : w - 1;
	p = src[coord_to_map_idx(y, x, 0, w, 1)];
	*r++ = p >> 11;
	*g++ = (p >> 5) & 0x3f;
	*b++ = p & 0x1f;

	x = xi < w ? xi : w - 1;
	y = yi + 1 < h ? yi + 1 : h - 1;
	p = src[coord_to_map_idx(y, x, 0, w, 1)];
	*r++ = p >> 11;
	*g++ = (p >> 5) & 0x3f;
	*b++ = p & 0x1f;

	x = xi + 1 < w ? xi + 1 : w - 1;
	p = src[coord_to_map_idx(y, x, 0, w, 1)];
	*r++ = p >> 11;
	*g++ = (p >> 5) & 0x3f;
	*b++ = p & 0x1f;
}

static void read_rgb888_quad(uint8_t *src_in, int xi, int yi, int w, int h, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint8_t *src;
	int x, y;

	x = xi < w ? xi : w - 1;
	y = yi < h ? yi : h - 1;
	src = src_in + coord_to_map_idx(y, x, 0, w, 3);
	*r++ = *src++;
	*g++ = *src++;
	*b++ = *src++;

	x = xi + 1 < w ? xi + 1 : w - 1;
	src = src_in + coord_to_map_idx(y, x, 0, w, 3);
	*r++ = *src++;
	*g++ = *src++;
	*b++ = *src++;

	x = xi < w ? xi : w - 1;
	y = yi + 1 < h ? yi + 1 : h - 1;
	src = src_in + coord_to_map_idx(y, x, 0, w, 3);
	*r++ = *src++;
	*g++ = *src++;
	*b++ = *src++;

	x = xi + 1 < w ? xi + 1 : w - 1;
	src = src_in + coord_to_map_idx(y, x, 0, w, 3);
	*r++ = *src++;
	*g++ = *src++;
	*b++ = *src++;
}

static uint8_t bilinear_grayscale(uint8_t *src, int x, int y, float dx, float dy, int w, int h)
{
	uint8_t value[4];

	read_grayscale_quad(src, x, y, w, h, value);

	return interpolate(value, dx, dy);
}

static uint16_t bilinear_rgb565(uint16_t *src, int x, int y, float dx, float dy, int w, int h)
{
	uint8_t red[4], green[4], blue[4];
	uint8_t r, g, b;

	read_rgb565_quad(src, x , y, w, h, red, green ,blue);

	r = clamp(interpolate(red, dx, dy), 0x1f);
	g = clamp(interpolate(green, dx, dy), 0x3f);
	b = clamp(interpolate(blue, dx, dy), 0x1f);

	return (r << 11) + (g << 5) + b;
}

static void bilinear_rgb888(uint8_t *src, int x, int y, float dx, float dy, int w, int h, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint8_t red[4], green[4], blue[4];

	read_rgb888_quad(src, x , y, w, h, red, green ,blue);

	*r = interpolate(red, dx, dy);
	*g = interpolate(green, dx, dy);
	*b = interpolate(blue, dx, dy);
}

static void dpel_nearest_rgb888(void *ctx, int c, int r, float u_s, float v_s)
{
	struct dpel_ctx *dctx = (struct dpel_ctx *) ctx;
	int x = (int)(u_s + 0.5);
	int y = (int)(v_s + 0.5);
	uint8_t *src;
	uint8_t *dst;

	src = dctx->isrc->data + coord_to_map_idx(y, x, 0, dctx->isrc->w, 3);
	dst = dctx->idst->data + coord_to_map_idx(r, c, 0, dctx->idst->w, 3);
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
}

static void dpel_bilinear_rgb888(void *ctx, int c, int r, float u_s, float v_s)
{
	struct dpel_ctx *dctx = (struct dpel_ctx *) ctx;
	uint8_t red, green, blue;
	int x = (int)u_s;
	int y = (int)v_s;
	float dx = u_s - x;
	float dy = v_s - y;
	uint8_t *dst;

	bilinear_rgb888(dctx->isrc->data, x, y, dx, dy, dctx->isrc->w, dctx->isrc->h, &red, &green, &blue);
	dst = dctx->idst->data + coord_to_map_idx(r, c, 0, dctx->idst->w, 3);
	*dst++ = red;
	*dst++ = green;
	*dst++ = blue;
}

static void dpel_nearest_rgb565(void *ctx, int c, int r, float u_s, float v_s)
{
	struct dpel_ctx *dctx = (struct dpel_ctx *) ctx;
	uint16_t *src = (uint16_t *) dctx->isrc->data;
	uint16_t *dst = (uint16_t *) dctx->idst->data;
	int x = (int)(u_s + 0.5);
	int y = (int)(v_s + 0.5);

	dst[coord_to_map_idx(r, c, 0, dctx->idst->w, 1)] = src[coord_to_map_idx(y, x, 0, dctx->isrc->w, 1)];
}

static void dpel_bilinear_rgb565(void *ctx, int c, int r, float u_s, float v_s)
{
	struct dpel_ctx *dctx = (struct dpel_ctx *) ctx;
	uint16_t *src = (uint16_t *) dctx->isrc->data;
	uint16_t *dst = (uint16_t *) dctx->idst->data;
	int x = (int)u_s;
	int y = (int)v_s;
	float dx = u_s - x;
	float dy = v_s - y;

	dst[coord_to_map_idx(r, c, 0, dctx->idst->w, 1)] = bilinear_rgb565(src, x, y, dx, dy, dctx->isrc->w, dctx->isrc->h);
}

static void dpel_nearest_grayscale(void *ctx, int c, int r, float u_s, float v_s)
{
	struct dpel_ctx *dctx = (struct dpel_ctx *) ctx;
	uint8_t *src = (uint8_t *) dctx->isrc->data;
	uint8_t *dst = (uint8_t *) dctx->idst->data;
	int x = (int)(u_s + 0.5);
	int y = (int)(v_s + 0.5);

	dst[coord_to_map_idx(r, c, 0, dctx->idst->w, 1)] = src[coord_to_map_idx(y, x, 0, dctx->isrc->w, 1)];
}

static void dpel_bilinear_grayscale(void *ctx, int c, int r, float u_s, float v_s)
{
	struct dpel_ctx *dctx = (struct dpel_ctx *) ctx;
	uint8_t *src = (uint8_t *) dctx->isrc->data;
	uint8_t *dst = (uint8_t *) dctx->idst->data;
	int x = (int)u_s;
	int y = (int)v_s;
	float dx = u_s - x;
	float dy = v_s - y;

	dst[coord_to_map_idx(r, c, 0, dctx->idst->w, 1)] = bilinear_grayscale(src, x, y, dx, dy, dctx->isrc->w, dctx->isrc->h);
}

static void dewarping_sparse_float_triangle(const image_t *isrc, image_t *idst, const mapxy_t *mapxy, const int idx[3],
										    draw_dewarp_pel dpel)
{
	struct dpel_ctx ctx = {
		.isrc = isrc,
		.idst = idst
	};
	float u[3];
	float v[3];
	int x[3];
	int y[3];
	int i;

	for (i = 0; i < 3; i++) {
		x[i] = mapxy->sparse_float.vertices[coord_to_map_idx(0, idx[i], 0, 0, 2)];
		y[i] = mapxy->sparse_float.vertices[coord_to_map_idx(0, idx[i], 1, 0, 2)];
		u[i] = mapxy->sparse_float.uv[coord_to_map_idx(0, idx[i], 0, 0, 2)];
		v[i] = mapxy->sparse_float.uv[coord_to_map_idx(0, idx[i], 1, 0, 2)];
	}

	draw_dewarp_triangle_fill(x[0], y[0], u[0], v[0], x[1], y[1], u[1], v[1], x[2], y[2], u[2], v[2], dpel, &ctx);
}

/* rgb888 */
static void dewarping_dense_float_nearest_rgb888(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = isrc->data;
	uint8_t *dst = idst->data;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)(mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)] + 0.5);
			y = (int)(mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)] + 0.5);

			*dst++ = src[coord_to_map_idx(y, x, 0, isrc->w, 3)];
			*dst++ = src[coord_to_map_idx(y, x, 1, isrc->w, 3)];
			*dst++ = src[coord_to_map_idx(y, x, 2, isrc->w, 3)];
		}
	}
}

static void dewarping_dense_float_bilinear_rgb888(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = isrc->data;
	uint8_t *dst = idst->data;
	uint8_t red, green, blue;
	float dx, dy;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)];
			y = (int)mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)];
			dx = mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)] - x;
			dy = mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)] - y;

			bilinear_rgb888(src, x, y, dx, dy, isrc->w, isrc->h, &red, &green, &blue);
			*dst++ = red;
			*dst++ = green;
			*dst++ = blue;
		}
	}
}

static void dewarping_dense_fp_12_4_nearest_rgb888(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = isrc->data;
	uint8_t *dst = idst->data;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)((mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)] + 8) / 16.0);
			y = (int)((mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)] + 8) / 16.0);

			*dst++ = src[coord_to_map_idx(y, x, 0, isrc->w, 3)];
			*dst++ = src[coord_to_map_idx(y, x, 1, isrc->w, 3)];
			*dst++ = src[coord_to_map_idx(y, x, 2, isrc->w, 3)];
		}
	}
}

static void dewarping_dense_fp_12_4_bilinear_rgb888(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = isrc->data;
	uint8_t *dst = idst->data;
	uint8_t red, green, blue;
	float dx, dy;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)]) >> 4;
			y = (mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)]) >> 4;
			dx = ((mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)]) & 0xf) / 16.0;
			dy = ((mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)]) & 0xf) / 16.0;

			bilinear_rgb888(src, x, y, dx, dy, isrc->w, isrc->h, &red, &green, &blue);
			*dst++ = red;
			*dst++ = green;
			*dst++ = blue;
		}
	}
}

static void dewarping_sparse_float_nearest_rgb888(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	const int *tri_idx = mapxy->sparse_float.tri_idx;
	int idx = 0;

	while (tri_idx[idx] >= 0) {
		dewarping_sparse_float_triangle(isrc, idst, mapxy, &tri_idx[idx], dpel_nearest_rgb888);
		idx += 3;
	}
}

static void dewarping_sparse_float_bilinear_rgb888(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	const int *tri_idx = mapxy->sparse_float.tri_idx;
	int idx = 0;

	while (tri_idx[idx] >= 0) {
		dewarping_sparse_float_triangle(isrc, idst, mapxy, &tri_idx[idx], dpel_bilinear_rgb888);
		idx += 3;
	}
}

/* rgb565 */
static void dewarping_dense_float_nearest_rgb565(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint16_t *src = (uint16_t *)isrc->data;
	uint16_t *dst = (uint16_t *)idst->data;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)(mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)] + 0.5);
			y = (int)(mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)] + 0.5);

			*dst++ = src[coord_to_map_idx(y, x, 0, isrc->w, 1)];
		}
	}
}

static void dewarping_dense_float_bilinear_rgb565(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint16_t *src = (uint16_t *)isrc->data;
	uint16_t *dst = (uint16_t *)idst->data;
	float dx, dy;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)];
			y = (int)mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)];
			dx = mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)] - x;
			dy = mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)] - y;

			*dst++ = bilinear_rgb565(src, x, y, dx, dy, isrc->w, isrc->h);
		}
	}
}

static void dewarping_dense_fp_12_4_nearest_rgb565(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint16_t *src = (uint16_t *)isrc->data;
	uint16_t *dst = (uint16_t *)idst->data;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)((mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)] + 8) / 16.0);
			y = (int)((mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)] + 8) / 16.0);

			*dst++ = src[coord_to_map_idx(y, x, 0, isrc->w, 1)];
		}
	}
}

static void dewarping_dense_fp_12_4_bilinear_rgb565(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint16_t *src = (uint16_t *)isrc->data;
	uint16_t *dst = (uint16_t *)idst->data;
	float dx, dy;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)]) >> 4;
			y = (mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)]) >> 4;
			dx = ((mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)]) & 0xf) / 16.0;
			dy = ((mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)]) & 0xf) / 16.0;

			*dst++ = bilinear_rgb565(src, x, y, dx, dy, isrc->w, isrc->h);
		}
	}
}

static void dewarping_sparse_float_nearest_rgb565(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	const int *tri_idx = mapxy->sparse_float.tri_idx;
	int idx = 0;

	while (tri_idx[idx] >= 0) {
		dewarping_sparse_float_triangle(isrc, idst, mapxy, &tri_idx[idx], dpel_nearest_rgb565);
		idx += 3;
	}
}

static void dewarping_sparse_float_bilinear_rgb565(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	const int *tri_idx = mapxy->sparse_float.tri_idx;
	int idx = 0;

	while (tri_idx[idx] >= 0) {
		dewarping_sparse_float_triangle(isrc, idst, mapxy, &tri_idx[idx], dpel_bilinear_rgb565);
		idx += 3;
	}
}

/* grayscale */
static void dewarping_dense_float_nearest_grayscale(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = (uint8_t *)isrc->data;
	uint8_t *dst = (uint8_t *)idst->data;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)(mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)] + 0.5);
			y = (int)(mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)] + 0.5);

			*dst++ = src[coord_to_map_idx(y, x, 0, isrc->w, 1)];
		}
	}
}

static void dewarping_dense_float_bilinear_grayscale(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = (uint8_t *)isrc->data;
	uint8_t *dst = (uint8_t *)idst->data;
	float dx, dy;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)];
			y = (int)mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)];
			dx = mapxy->dense_float[coord_to_map_idx(r, c, 1, isrc->w, 2)] - x;
			dy = mapxy->dense_float[coord_to_map_idx(r, c, 0, isrc->w, 2)] - y;

			*dst++ = bilinear_grayscale(src, x, y, dx, dy, isrc->w, isrc->h);
		}
	}
}

static void dewarping_dense_fp_12_4_nearest_grayscale(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = (uint8_t *)isrc->data;
	uint8_t *dst = (uint8_t *)idst->data;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (int)((mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)] + 8) / 16.0);
			y = (int)((mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)] + 8) / 16.0);

			*dst++ = src[coord_to_map_idx(y, x, 0, isrc->w, 1)];
		}
	}
}

static void dewarping_dense_fp_12_4_bilinear_grayscale(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	uint8_t *src = (uint8_t *)isrc->data;
	uint8_t *dst = (uint8_t *)idst->data;
	float dx, dy;
	int r, c;
	int x, y;

	for (r = 0; r < isrc->h; r++) {
		for (c = 0; c < isrc->w; c++) {
			x = (mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)]) >> 4;
			y = (mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)]) >> 4;
			dx = ((mapxy->dense_fp[coord_to_map_idx(r, c, 1, isrc->w, 2)]) & 0xf) / 16.0;
			dy = ((mapxy->dense_fp[coord_to_map_idx(r, c, 0, isrc->w, 2)]) & 0xf) / 16.0;

			*dst++ = bilinear_grayscale(src, x, y, dx, dy, isrc->w, isrc->h);
		}
	}
}

static void dewarping_sparse_float_nearest_grayscale(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	const int *tri_idx = mapxy->sparse_float.tri_idx;
	int idx = 0;

	while (tri_idx[idx] >= 0) {
		dewarping_sparse_float_triangle(isrc, idst, mapxy, &tri_idx[idx], dpel_nearest_grayscale);
		idx += 3;
	}
}

static void dewarping_sparse_float_bilinear_grayscale(const image_t *isrc, image_t *idst, const mapxy_t *mapxy)
{
	const int *tri_idx = mapxy->sparse_float.tri_idx;
	int idx = 0;

	while (tri_idx[idx] >= 0) {
		dewarping_sparse_float_triangle(isrc, idst, mapxy, &tri_idx[idx], dpel_bilinear_grayscale);
		idx += 3;
	}
}

static stm32ipl_err_t dewarping_check_params_mapxy(const mapxy_t *mapxy)
{
	switch (mapxy->type) {
	case MAPXY_DENSE_FLOAT:
		return mapxy->dense_float ? stm32ipl_err_Ok : stm32ipl_err_InvalidParameter;
		break;
	case MAPXY_DENSE_FIXED_POINT_12_4:
		return mapxy->dense_fp ? stm32ipl_err_Ok : stm32ipl_err_InvalidParameter;
		break;
	case MAPXY_SPARSE_FLOAT:
		if (mapxy->sparse_float.vertices && mapxy->sparse_float.uv && mapxy->sparse_float.tri_idx)
			return stm32ipl_err_Ok;
		else
			return stm32ipl_err_InvalidParameter;
		break;
	default:
		return stm32ipl_err_InvalidParameter;
	}
}

static stm32ipl_err_t dewarping_check_params(const image_t *src, image_t *dst, const mapxy_t *mapxy, dewarping_algo_t algo)
{
	/* check null pointer first */
	if (!src || !dst || !mapxy || !src->data || !dst->data)
		return stm32ipl_err_InvalidParameter;

	/* check src and dst compat */
	if (src->w != dst->w || src->h != dst->h || src->bpp != dst->bpp || src->data == dst->data)
		return stm32ipl_err_InvalidParameter;

	if (algo != DEWARP_NEAREST && algo != DEWARP_BILINEAR)
		return stm32ipl_err_InvalidParameter;

	return dewarping_check_params_mapxy(mapxy);
}

static dewarp_fct dewarp_fct_implementations[IMAGE_BPP_NB][MAPXY_TYPE_NB][DEWARP_ALGO_NB] = {
	// IMAGE_BPP_BINARY => unsupported
	{{NULL, NULL }, {NULL, NULL }, {NULL, NULL }},
	// IMAGE_BPP_GRAYSCALE
	{
		{dewarping_dense_float_nearest_grayscale, dewarping_dense_float_bilinear_grayscale },
		{dewarping_dense_fp_12_4_nearest_grayscale, dewarping_dense_fp_12_4_bilinear_grayscale },
		{dewarping_sparse_float_nearest_grayscale, dewarping_sparse_float_bilinear_grayscale },
	},
	// IMAGE_BPP_RGB565
	{
		{dewarping_dense_float_nearest_rgb565, dewarping_dense_float_bilinear_rgb565 },
		{dewarping_dense_fp_12_4_nearest_rgb565, dewarping_dense_fp_12_4_bilinear_rgb565 },
		{dewarping_sparse_float_nearest_rgb565, dewarping_sparse_float_bilinear_rgb565 },
	},
	// IMAGE_BPP_BAYER => unsupported
	{{NULL, NULL }, {NULL, NULL }, {NULL, NULL }},
	// IMAGE_BPP_RGB888
	{
		{dewarping_dense_float_nearest_rgb888, dewarping_dense_float_bilinear_rgb888 },
		{dewarping_dense_fp_12_4_nearest_rgb888, dewarping_dense_fp_12_4_bilinear_rgb888 },
		{dewarping_sparse_float_nearest_rgb888, dewarping_sparse_float_bilinear_rgb888 },
	},
	// IMAGE_BPP_JPEG => unsupported
	{{NULL, NULL }, {NULL, NULL }, {NULL, NULL }},
};

/**
 * @brief Performs dewarping of src image
 * The supported formats are Grayscale, RGB565, RGB888.
 * @param src			Source image (image is warped due to camera lens)
 * @param dst			Destination image (dewarp image)
 * @param mapxy			mapping data to perform the dewarping
 * @param algo			perform either nearest or bilinear dewarping
 * src and dst must have the same resolution and same format.
 *
 * The general dense algorithm works as follows:
 *   For each destination pixel (x, y), we perform:
 *     dst[y][x] = src[dense_float[y][x][1]][dense_float[y][x][0]]
 *
 *   However, since the values in dense_float[y][x] are floating-point numbers, we cannot directly use them to access
 *   the source image at that position.
 *   In DEWARP_NEAREST mode, we use the pixel value nearest to this position.
 *   In DEWARP_BILINEAR mode, we use bilinear interpolation to compute the pixel value at this location.
 *
 * The sparse dense algorithm works as follows:
 *   For each pixel within each triangle:
 *     We interpolate the source image location to read by using the source pixel locations of the triangle's
 *     corners (Those value are given by uv array)
 *     Then, we apply the same algorithm as the dense method to compute the destination pixel value based on the
 *     interpolated source location.
 *
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Dewarp(const image_t *src, image_t *dst, const mapxy_t *mapxy, dewarping_algo_t algo)
{
	stm32ipl_err_t ret;

	ret = dewarping_check_params(src, dst, mapxy, algo);
	if (ret)
		return ret;

	if (dewarp_fct_implementations[src->bpp][mapxy->type][algo])
		dewarp_fct_implementations[src->bpp][mapxy->type][algo](src, dst, mapxy);
	else
		return stm32ipl_err_NotImplemented;

	return ret;
}

#ifdef __cplusplus
}
#endif
