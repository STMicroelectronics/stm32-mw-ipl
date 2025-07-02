/**
 ******************************************************************************
 * @file    mve_draw.c
 * @author  AIS Team
 * @brief   MVE Image processing library drawing functions with MVE intrinsics

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

#include "imlib.h"

#ifdef IPL_DRAW_HAS_MVE
#include "mve_draw.h"

void point_fill_mve_binary(image_t *img, int cx, int cy, int r0, int r1, int c)
{
  int r = r0;
  while ((r1 - r) > 0){
    int8x16_t x_x16 = vreinterpretq_s8_u8(vidupq_n_u8(0, 1));
    x_x16 = vaddq_n_s8(x_x16, r);
    x_x16 = vmulq_s8(x_x16, x_x16);
    /* Handle x pos cx + r cx + r + 16 */
    /* if < 0 set low bits to 0 */
    /* if cx + r + 16 >= img->w set hight bits to 0 */
    mve_pred16_t pred_pos = vctp8q(img->w-(cx + r));
    pred_pos = vctp8q_m(r1 - r + 1, pred_pos);

    if ((cx + r) < 0) pred_pos &= vpnot(vctp8q(-(cx + r)));

    for (int y = r0; y <= r1; y++) {
      /* mve_pred16_t p : 1 if (x * x) + (y * y)) <= (r0 * r0) */
      /*                    x * x <= (r0 * r0) - (y * y) */
      if ((cy + y >= 0) && (cy + y < img->h)) {
        int8x16_t dist_x16 = vqaddq_n_s8(x_x16, y * y);
        mve_pred16_t p = vcmpleq_m_n_s8(dist_x16, (r0 * r0), pred_pos);
        int x_pos = cx + r;
        while(p) {
          if (p&1) 
            IMAGE_PUT_BINARY_PIXEL(img, x_pos, cy + y, c);
          x_pos++;
          p >>= 1;
        }
      }
    }
  r += 16;
  }
}

void point_fill_mve_grayscale(image_t *img, int cx, int cy, int r0, int r1, int c)
{
  int r = r0;
  while ((r1 - r) > 0){
    int8x16_t x_x16 = vreinterpretq_s8_u8(vidupq_n_u8(0, 1));
    x_x16 = vaddq_n_s8(x_x16, r);
    x_x16 = vmulq_s8(x_x16, x_x16);
    /* Handle x pos cx + r0 cx + r0 + 16 */
    /* if < 0 set low bits to 0 */
    /* if cx + r0 + 16 >= img->w set hight bits to 0 */
    mve_pred16_t pred_pos = vctp8q(img->w-(cx + r));
    pred_pos = vctp8q_m(r1 - r + 1, pred_pos);

    if ((cx + r) < 0) pred_pos &= vpnot(vctp8q(-(cx + r)));

    for (int y = r0; y <= r1; y++) {
      /* mve_pred16_t p : 1 if (x * x) + (y * y)) <= (r0 * r0) */
      /*                    x * x <= (r0 * r0) - (y * y) */
      if ((cy + y >= 0) && (cy + y < img->h)) {
        int8x16_t dist_x16 = vqaddq_n_s8(x_x16, y * y);
        mve_pred16_t p = vcmpleq_m_n_s8(dist_x16, (r0 * r0), pred_pos);
        vstrbq_p_u8(IMAGE_GET_GRAYSCALE_PIXEL_ADDR(img, cx + r, cy + y), vdupq_n_u8(c&0xFF), p);
      }
    }
    r += 16;
  }
}

void point_fill_mve_rgb888(image_t *img, int cx, int cy, int r0, int r1, int c)
{
  uint8x16_t offset = vidupq_n_u8(0, 1) * 3;
  int r = r0;
  while ((r1 - r) >= 0){
    int8x16_t x_x16 = vreinterpretq_s8_u8(vidupq_n_u8(0, 1));
    x_x16 = vaddq_n_s8(x_x16, r);
    x_x16 = vmulq_s8(x_x16, x_x16);

    /* Handle x pos cx + r0 cx + r0 + 16 */
    /* if < 0 set low bits to 0 */
    /* if cx + r0 + 16 >= img->w set hight bits to 0 */
    mve_pred16_t pred_pos = vctp8q(img->w-(cx + r));
    pred_pos = vctp8q_m(r1 - r + 1, pred_pos);
    
    if ((cx + r) < 0) pred_pos &= vpnot(vctp8q(-(cx + r)));

    for (int y = r0; y <= r1; y++) {
      /* mve_pred16_t p : 1 if (x * x) + (y * y)) <= (r0 * r0) */
      /*                    x * x <= (r0 * r0) - (y * y) */
      if ((cy + y >= 0) && (cy + y < img->h)) {
        int8x16_t dist_x16 = vqaddq_n_s8(x_x16, y * y);
        mve_pred16_t p = vcmpleq_m_n_s8(dist_x16, (r0 * r0), pred_pos);
        rgb888_t *rgb888 = IMAGE_GET_RGB888_PIXEL_ADDR(img, cx + r, cy + y);
        vstrbq_scatter_offset_p_u8(&(rgb888->r), offset, vdupq_n_u8((c >> 16) & 0xFF), p);
        vstrbq_scatter_offset_p_u8(&(rgb888->g), offset, vdupq_n_u8((c >>  8) & 0xFF), p);
        vstrbq_scatter_offset_p_u8(&(rgb888->b), offset, vdupq_n_u8( c        & 0xFF), p);
      }
    }
    r += 16;
  }
}

void point_fill_mve_rgb565(image_t *img, int cx, int cy, int r0, int r1, int c)
{
  int r = r0;
  while ((r1 - r) > 0){
    int16x8_t x_x8 = vreinterpretq_s16_u16(vidupq_n_u16(0, 1));
    x_x8 = vaddq_n_s16(x_x8, r);
    x_x8 = vmulq_s16(x_x8, x_x8);
    /* Handle x pos cx + r0 cx + r0 + 16 */
    /* if < 0 set low bits to 0 */
    /* if cx + r0 + 16 >= img->w set hight bits to 0 */
    mve_pred16_t pred_pos = vctp16q(img->w - (cx + r));
    pred_pos = vctp16q_m(r1 - r + 1, pred_pos);

    if ((cx + r) < 0) pred_pos &= vpnot(vctp16q(-(cx + r)));

    for (int y = r0; y <= r1; y++) {
      /* mve_pred16_t p : 1 if (x * x) + (y * y)) <= (r0 * r0) */
      /*                    x * x <= (r0 * r0) - (y * y) */
      if ((cy + y >= 0) && (cy + y < img->h)) {
        int16x8_t dist_x8 = vqaddq_n_s16(x_x8, y * y);
        mve_pred16_t p = vcmpleq_m_n_s16(dist_x8, r0 * r0, pred_pos);
        vstrhq_p_u16(IMAGE_GET_RGB565_PIXEL_ADDR(img, cx + r, cy + y), vdupq_n_u16(c), p);
      }
    }
    r += 8;
  }
}
#endif /* IPL_DRAW_HAS_MVE */
