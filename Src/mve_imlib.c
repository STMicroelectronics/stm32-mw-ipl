/**
 ******************************************************************************
 * @file    mve_imlib.c
 * @author  AIS Team
 * @brief   MVE Image processing library imlib MVE functions
 
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
#include "common.h"

#ifdef IPL_IMLIB_HAS_MVE
#include "mve_imlib.h"

static inline mve_pred16_t mve_image_get_mask_pixel_binary(image_t *ptr, int x, int y)
{
  mve_pred16_t pred = vctp8q(ptr->w - x);
  if ((x&~0x1F) == 0) {
    pred &= (mve_pred16_t)(*(uint32_t *)IMAGE_GET_BINARY_PIXEL_ADDR(ptr, x, y) & 0xFFFF);
  } else if ((x&~0xF) == 0) {
    pred &= (mve_pred16_t)(*(uint32_t *)IMAGE_GET_BINARY_PIXEL_ADDR(ptr, x, y) & 0xFFFF);
  } else {
    for (int i = x; i < IM_MIN(x + 16, ptr->w); i++) {
      pred &= IMAGE_GET_BINARY_PIXEL(ptr, i, y) << i;
    }
  }
  return pred;
}

static inline mve_pred16_t mve_image_get_mask_pixel_grayscale(image_t *ptr, int x, int y)
{
  mve_pred16_t pred = vctp8q(ptr->w - x);
  uint8_t *ptr_u8 = IMAGE_GET_GRAYSCALE_PIXEL_ADDR(ptr, x, y);
  uint8x16_t data = vldrbq_z_u8(ptr_u8 , pred);
  pred = vcmphiq_m_u8(data, vdupq_n_u8(((COLOR_GRAYSCALE_MAX - COLOR_GRAYSCALE_MIN) / 2) + COLOR_GRAYSCALE_MIN), pred);
  return pred;
}

static inline mve_pred16_t mve_image_get_mask_pixel_rgb565(image_t *ptr, int x, int y)
{
  mve_pred16_t pred = vctp16q(ptr->w - x);
  uint16x8_t u16x8_r, u16x8_g, u16x8_b;
  /* Load u16 x8 */
  uint16x8_t data = vldrhq_z_u16(IMAGE_GET_RGB565_PIXEL_ADDR(ptr, x, y), pred);
  /* compute u16 x2 */
  /*__pixel = (__pixel >> 8) & 0xF8; */
  /* MVE RGB565 to RGB888 */
  u16x8_r = vandq_u16(vshrq_n_u16(data, 8), vdupq_n_u16(0xF8));
  /*__pixel = (__pixel >> 3) & 0xFC; */
  u16x8_g = vandq_u16(vshrq_n_u16(data, 3), vdupq_n_u16(0xFC));
  /*__pixel = (__pixel << 3) & 0xF8; */
  u16x8_b = vandq_u16(vshlq_n_u16(data, 3), vdupq_n_u16(0xF8));
  /* RGB888 to Y */
  data = vmulq_n_u16(u16x8_r, 38);
  data = vmlaq_n_u16(data, u16x8_g, 75);
  data = vmlaq_n_u16(data, u16x8_b, 15);
  data = vshrq_n_u16(data, 7);
  /* Compare to average */
  pred = vcmphiq_m_u16(data, vdupq_n_u16(((COLOR_Y_MAX - COLOR_Y_MIN) / 2) + COLOR_Y_MIN), pred);
  return pred;
}

static inline mve_pred16_t mve_image_get_mask_pixel_rgb888(image_t *ptr, int x, int y)
{
  mve_pred16_t pred = vctp16q(ptr->w - x);
  uint8x16_t u8x16_incr = vidupq_n_u8(0, 1) * 3;
  uint16x8_t u16x8_r, u16x8_g, u16x8_b, u16x8_data;
  uint8x16_t u8x16_data = {0};
  rgb888_t *ptr_rgb888 = IMAGE_GET_RGB888_PIXEL_ADDR(ptr, x, y);
  uint8x16_t u8x16_r, u8x16_g, u8x16_b;
  /* load RGB data */
  u8x16_r = vldrbq_gather_offset_z_u8(&(ptr_rgb888->r), u8x16_incr, pred);
  u8x16_g = vldrbq_gather_offset_z_u8(&(ptr_rgb888->g), u8x16_incr, pred);
  u8x16_b = vldrbq_gather_offset_z_u8(&(ptr_rgb888->b), u8x16_incr, pred);
  /* u8 tu u16: even */
  u16x8_r = vmovlbq_u8(u8x16_r);
  u16x8_g = vmovlbq_u8(u8x16_g);
  u16x8_b = vmovlbq_u8(u8x16_b);
  u16x8_data = vmulq_n_u16(u16x8_r, 38);
  u16x8_data = vmlaq_n_u16(u16x8_data, u16x8_g, 75);
  u16x8_data = vmlaq_n_u16(u16x8_data, u16x8_b, 15);
  u16x8_data = vshrq_n_u16(u16x8_data, 7);
  u8x16_data = vmovnbq_u16(u8x16_data, u16x8_data);
  /* u8 tu u16: odd */
  u16x8_r = vmovltq_u8(u8x16_r);
  u16x8_g = vmovltq_u8(u8x16_g);
  u16x8_b = vmovltq_u8(u8x16_b);
  u16x8_data = vmulq_n_u16(u16x8_r, 38);
  u16x8_data = vmlaq_n_u16(u16x8_data, u16x8_g, 75);
  u16x8_data = vmlaq_n_u16(u16x8_data, u16x8_b, 15);
  u16x8_data = vshrq_n_u16(u16x8_data, 7);
  u8x16_data = vmovntq_u16(u8x16_data, u16x8_data);
  /* Compare */
  pred = vcmphiq_m_u8(u8x16_data, vdupq_n_u8(((COLOR_Y_MAX - COLOR_Y_MIN) / 2) + COLOR_Y_MIN), pred);
  return pred;
}

mve_pred16_t mve_image_get_mask_pixel(image_t *ptr, int x, int y)
{
  mve_pred16_t pred = 0;
  switch (ptr->bpp) {
    case IMAGE_BPP_BINARY: {
      pred = mve_image_get_mask_pixel_binary(ptr, x, y);
        break;
    }
    case IMAGE_BPP_GRAYSCALE: {
      pred = mve_image_get_mask_pixel_grayscale(ptr, x, y);
      break;
    }
    case IMAGE_BPP_RGB565: {
      pred = mve_image_get_mask_pixel_rgb565(ptr, x, y);
      break;
    }
    case IMAGE_BPP_RGB888: { /* STM32IPL */
      pred = mve_image_get_mask_pixel_rgb888(ptr, x, y);
      break;
    }
    default: {
        break;
    }
  }
    return pred;
}
#endif /* IPL_IMLIB_HAS_MVE */