/**
 ******************************************************************************
 * @file    mve_binary.c
 * @author  AIS Team
 * @brief   MVE Image processing library binary functions

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

#ifdef IPL_BINARY_HAS_MVE
#include "mve_binary.h"
#include "mve_imlib.h"


static inline uint16x8_t _mve_color_r8_g8_b8_to_rgb565(uint16x8_t u16x8_r8, uint16x8_t u16x8_g8, uint16x8_t u16x8_b8) {
  return (vshlq_n_u16(vandq_u16(u16x8_r8, vdupq_n_u16(0xF8)), 8) |
          vshlq_n_u16(vandq_u16(u16x8_g8, vdupq_n_u16(0xFC)), 3) |
          vshrq_n_u16(u16x8_b8, 3));
}

static inline void _mve_color_rgb565_to_lab(uint16x8_t u16x8_pixel_b, uint16x8_t u16x8_pixel_t, uint8x16_t *l, int8x16_t *a, int8x16_t *b) {
  uint16x8_t l_16x8, u16x8_pixel;
  int16x8_t a_16x8, b_16x8;
  uint32x4_t u32x4_pixel_b, u32x4_pixel_t;
  u16x8_pixel = vshrq_n_u16(u16x8_pixel_b, 1);
  /* (u16_pixel >> 1) * 3 can be larger than uint16_t range
   * use u32 to handle it (uint32x4_t) */
  u32x4_pixel_b = vmovlbq_u16(u16x8_pixel) * 3;
  u32x4_pixel_t = vmovltq_u16(u16x8_pixel) * 3;
  l_16x8 = vmovntq_u32(vreinterpretq_u16_u32(vldrbq_gather_offset_u32((uint8_t*)(lab_table + 0), u32x4_pixel_b)),
                       vldrbq_gather_offset_u32((uint8_t*)(lab_table + 0), u32x4_pixel_t));
  a_16x8 = vmovntq_s32(vreinterpretq_s16_s32(vldrbq_gather_offset_s32(lab_table + 1, u32x4_pixel_b)),
                       vldrbq_gather_offset_s32((lab_table + 1), u32x4_pixel_t));
  b_16x8 = vmovntq_s32(vreinterpretq_s16_s32(vldrbq_gather_offset_s32(lab_table + 2, u32x4_pixel_b)),
                       vldrbq_gather_offset_s32((lab_table + 2), u32x4_pixel_t));
  *l = vmovnbq_u16(*l, l_16x8);
  *a = vmovnbq_s16(*a, a_16x8);
  *b = vmovnbq_s16(*b, b_16x8);
  u16x8_pixel = vshrq_n_u16(u16x8_pixel_t, 1);
  /* (u16_pixel >> 1) * 3 can be larger than uint16_t range
   * use u32 to handle it (uint32x4_t) */
  u32x4_pixel_b = vmovlbq_u16(u16x8_pixel) * 3;
  u32x4_pixel_t = vmovltq_u16(u16x8_pixel) * 3;
  l_16x8 = vmovntq_u32(vreinterpretq_u16_u32(vldrbq_gather_offset_u32((uint8_t*)(lab_table + 0), u32x4_pixel_b)),
                       vldrbq_gather_offset_u32((uint8_t*)(lab_table + 0), u32x4_pixel_t));
  a_16x8 = vmovntq_s32(vreinterpretq_s16_s32(vldrbq_gather_offset_s32(lab_table + 1, u32x4_pixel_b)),
                       vldrbq_gather_offset_s32((lab_table + 1), u32x4_pixel_t));
  b_16x8 = vmovntq_s32(vreinterpretq_s16_s32(vldrbq_gather_offset_s32(lab_table + 2, u32x4_pixel_b)),
                       vldrbq_gather_offset_s32((lab_table + 2), u32x4_pixel_t));
  *l = vmovntq_u16(*l, l_16x8);
  *a = vmovntq_s16(*a, a_16x8);
  *b = vmovntq_s16(*b, b_16x8);
}
static inline mve_pred16_t _mve_color_threshold_lab( uint8x16_t l, int8x16_t a, int8x16_t b, color_thresholds_list_lnk_data_t *threshold) {
  return (vcmpcsq_n_u8(l, (threshold)->LMin) & vcmpcsq_u8(vdupq_n_u8((threshold)->LMax), l) &
          vcmpgeq_n_s8(a, (threshold)->LMin) & vcmpleq_n_s8(a, (threshold)->AMax) &
          vcmpgeq_n_s8(b, (threshold)->LMin) & vcmpleq_n_s8(b, (threshold)->BMax));
}

static inline void _mve_color_rgb565_to_lab16(uint16x8_t u16x8_pixel_in,  uint16x8_t *l, int16x8_t *a, int16x8_t *b) {
  uint16x8_t u16x8_pixel;
  uint32x4_t u32x4_pixel_b, u32x4_pixel_t;
  u16x8_pixel = vshrq_n_u16(u16x8_pixel_in, 1);
  u32x4_pixel_b = vmovlbq_u16(u16x8_pixel) * 3;
  u32x4_pixel_t = vmovltq_u16(u16x8_pixel) * 3;
  /* (u16_pixel >> 1) * 3 can be larger than uint16_t range
   * use u32 to handle it (uint32x4_t) */
  *l = vmovntq_u32(vreinterpretq_u16_u32(vldrbq_gather_offset_u32((uint8_t*)(lab_table + 0), u32x4_pixel_b)),
                   vldrbq_gather_offset_u32((uint8_t*)(lab_table + 0), u32x4_pixel_t));
  *a = vmovntq_s32(vreinterpretq_s16_s32(vldrbq_gather_offset_s32(lab_table + 1, u32x4_pixel_b)),
                   vldrbq_gather_offset_s32((lab_table + 1), u32x4_pixel_t));
  *b = vmovntq_s32(vreinterpretq_s16_s32(vldrbq_gather_offset_s32(lab_table + 2, u32x4_pixel_b)),
                   vldrbq_gather_offset_s32((lab_table + 2), u32x4_pixel_t));
}
static inline mve_pred16_t __mve_color_threshold_lab16( uint16x8_t l, int16x8_t a, int16x8_t b, color_thresholds_list_lnk_data_t *threshold) {
  return (vcmpcsq_n_u16(l, (threshold)->LMin) & vcmpcsq_u16(vdupq_n_u16((threshold)->LMax), l) &
          vcmpgeq_n_s16(a, (threshold)->AMin) & vcmpleq_n_s16(a, (threshold)->AMax) &
          vcmpgeq_n_s16(b, (threshold)->BMin) & vcmpleq_n_s16(b, (threshold)->BMax));
}

static inline void _mve_rgb888_u8x16_to_2_u16x8(uint8x16_t u8x16_r, uint8x16_t u8x16_g, uint8x16_t u8x16_b, uint16x8_t *u16x8_data_b, uint16x8_t *u16x8_data_t) {
  uint16x8_t u16x8_r, u16x8_g, u16x8_b;
  /* even */
  u16x8_r = vmovlbq_u8(u8x16_r);
  u16x8_g = vmovlbq_u8(u8x16_g);
  u16x8_b = vmovlbq_u8(u8x16_b);
  *u16x8_data_b = _mve_color_r8_g8_b8_to_rgb565(u16x8_r, u16x8_g, u16x8_b);
  /* odd */
  u16x8_r = vmovltq_u8(u8x16_r);
  u16x8_g = vmovltq_u8(u8x16_g);
  u16x8_b = vmovltq_u8(u8x16_b);
  *u16x8_data_t = _mve_color_r8_g8_b8_to_rgb565(u16x8_r, u16x8_g, u16x8_b);
}

static inline mve_pred16_t _mve_rgb888_to_binary(uint8x16_t u8x16_r, uint8x16_t u8x16_g, uint8x16_t u8x16_b) {
  uint16x8_t u16x8_r, u16x8_g, u16x8_b;
  mve_pred16_t pred;
  uint8x16_t u8x16_y = {0};
  /* even */
  u16x8_r = vmovlbq_u8(u8x16_r);
  u16x8_g = vmovlbq_u8(u8x16_g);
  u16x8_b = vmovlbq_u8(u8x16_b);
  u8x16_y = vmovnbq_u16(u8x16_y, (u16x8_r * 38 + u16x8_g * 75 + u16x8_b * 15) >> 7);
  /* odd */
  u16x8_r = vmovltq_u8(u8x16_r);
  u16x8_g = vmovltq_u8(u8x16_g);
  u16x8_b = vmovltq_u8(u8x16_b);
  u8x16_y = vmovntq_u16(u8x16_y, (u16x8_r * 38 + u16x8_g * 75 + u16x8_b * 15) >> 7);
  pred = vcmphiq_u8(u8x16_y, vdupq_n_u8(((COLOR_Y_MAX - COLOR_Y_MIN) / 2) + COLOR_Y_MIN));
  return pred;
}

static inline mve_pred16_t _mve_rgb565_to_binary(uint16x8_t u16x8_rgb_b, uint16x8_t u16x8_rgb_t) {
  mve_pred16_t pred;
  uint8x16_t u8x16_y = {0};
  /* even */
  u8x16_y = vmovnbq_u16(u8x16_y, (vshrq_n_u16(vandq_u16(u16x8_rgb_b,vdupq_n_u16(0xF800)),8) * 38 \
          + vshrq_n_u16(vandq_u16(u16x8_rgb_b,vdupq_n_u16(0x07E0)),3) * 75 \
          + vshlq_n_u16(vandq_u16(u16x8_rgb_b,vdupq_n_u16(0x001F)),3) * 15) >> 7);
  /* odd */
  u8x16_y = vmovntq_u16(u8x16_y, (vshrq_n_u16(vandq_u16(u16x8_rgb_t,vdupq_n_u16(0xF800)),8) * 38 \
          + vshrq_n_u16(vandq_u16(u16x8_rgb_t,vdupq_n_u16(0x07E0)),3) * 75 \
          + vshlq_n_u16(vandq_u16(u16x8_rgb_t,vdupq_n_u16(0x001F)),3) * 15) >> 7);
  pred = vcmphiq_u8(u8x16_y, vdupq_n_u8(((COLOR_Y_MAX - COLOR_Y_MIN) / 2) + COLOR_Y_MIN));
  return pred;
}

/* Grayscale to binary */
/***********************/
static inline void mve_imlib_binary_grayscale_out_binary_fast(image_t *out, 
                                                              image_t *img,
                                                              color_thresholds_list_lnk_data_t *threshold)
{
  for (int y = 0; y < img->h; y++) {
    uint8_t *old_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = (uint16_t *) IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(out, y);
    for (int x = 0; x < img->w; x += 16) {
      mve_pred16_t p = vctp8q(img->w - x);
      uint8x16_t u8x16_read = vldrbq_z_u8(old_row_ptr + x, p);
      mve_pred16_t pred;
      /* LMin <= read <= LMax */
      pred = vcmpcsq_n_u8(u8x16_read, threshold->LMin); /* u8x16_read >= lnk_data.LMin */
      pred &= vcmpcsq_u8(vdupq_n_u8(threshold->LMax), u8x16_read); /* lnk_data.LMax >= u8x16_read */
      *out_row_ptr++ = pred; /* 16bits each time */
    }
  }
}

static inline void mve_imlib_binary_grayscale_out_binary(image_t *out, 
                                                         image_t *img,
                                                         color_thresholds_list_lnk_data_t *threshold,
                                                         bool invert,
                                                         bool zero,
                                                         image_t *mask)
{
  for (int y = 0; y < img->h; y++) {
    uint8_t *old_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = (uint16_t *) IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(out, y);
    for (int x = 0; x < img->w; x += 16) {
      mve_pred16_t p = vctp8q(img->w - x);
      uint8x16_t u8x16_read = vldrbq_z_u8(old_row_ptr + x, p);
      mve_pred16_t pred, p_mask, data_binary;
      data_binary = vcmpcsq_n_u8(u8x16_read,
                           (((COLOR_GRAYSCALE_MAX - COLOR_GRAYSCALE_MIN) / 2) + COLOR_GRAYSCALE_MIN));
      p_mask = (mask != NULL) ? mve_image_get_mask_pixel(mask, x, y) : 0xFFFF;
      pred = vcmpcsq_n_u8(u8x16_read, threshold->LMin); /* u8x16_read >= lnk_data.LMin */
      pred &= vcmpcsq_u8(vdupq_n_u8(threshold->LMax), u8x16_read); /* lnk_data.LMax >= u8x16_read */
      if (invert) {
        pred ^= 0xFFFF;
      }
      if (zero) {
        pred &= p_mask;
        data_binary &= ~pred;
      } else {
        data_binary = (data_binary & ~p_mask) | (pred & p_mask);
      }
      *out_row_ptr++ = data_binary; /* 16bits each time */
    }
  }
}
static inline void mve_imlib_binary_grayscale_out_grayscale_fast(image_t *out, 
                                                                 image_t *img,
                                                                 color_thresholds_list_lnk_data_t *threshold)
{
  for (int y = 0; y < img->h; y++) {
    uint8_t *old_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
    uint8_t *out_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p = vctp8q(x);
      uint8x16_t u8x16_out;
      uint8x16_t u8x16_read = vldrbq_z_u8(old_row_ptr, p);
      mve_pred16_t pred;
      pred = vcmpcsq_n_u8(u8x16_read, threshold->LMin); /* u8x16_read >= lnk_data.LMin */
      pred &= vcmpcsq_u8(vdupq_n_u8(threshold->LMax), u8x16_read); /* lnk_data.LMax >= u8x16_read */

      u8x16_out = vpselq_u8(
                  vdupq_n_u8(COLOR_BINARY_TO_GRAYSCALE(1)),
                  vdupq_n_u8(COLOR_BINARY_TO_GRAYSCALE(0)), 
                  pred);
      vstrbq_p_u8(out_row_ptr, u8x16_out, p);

      out_row_ptr += 16;
      old_row_ptr += 16;
      x -= 16;
    }
  }
}

static inline void mve_imlib_binary_grayscale_out_grayscale(image_t *out, 
                                                            image_t *img,
                                                            color_thresholds_list_lnk_data_t *threshold,
                                                            bool invert,
                                                            bool zero,
                                                            image_t *mask)
{
  for (int y = 0; y < img->h; y++) {
    uint8_t *old_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
    uint8_t *out_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(out, y);
    for (int x = 0; x < img->w; x += 16) {
      mve_pred16_t p = vctp8q(img->w - x);
      uint8x16_t u8x16_out;
      uint8x16_t u8x16_read = vldrbq_z_u8(old_row_ptr + x, p);
      mve_pred16_t pred, p_mask;
      p_mask = (mask != NULL) ? mve_image_get_mask_pixel(mask, x, y) : 0xFFFF;
      pred = vcmpcsq_n_u8(u8x16_read, threshold->LMin); /* u8x16_read >= lnk_data.LMin */
      pred &= vcmpcsq_u8(vdupq_n_u8(threshold->LMax), u8x16_read); /* lnk_data.LMax >= u8x16_read */
      if (invert) {
        pred ^= 0xFFFF;
      }
      if (zero) {
        pred &= p_mask;
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(COLOR_BINARY_TO_GRAYSCALE(0)),
                        u8x16_read,
                        pred);
      } else {
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(COLOR_BINARY_TO_GRAYSCALE(1)),
                        vdupq_n_u8(COLOR_BINARY_TO_GRAYSCALE(0)),
                        pred);
        u8x16_out = vpselq_u8(u8x16_out, u8x16_read, p_mask);
      }
      vstrbq_p_u8(out_row_ptr, u8x16_out, p);
      out_row_ptr += 16;
    }
  }
}

void mve_imlib_binary_grayscale(image_t *out, 
                                image_t *img,
                                color_thresholds_list_lnk_data_t *threshold,
                                bool invert,
                                bool zero,
                                image_t *mask)
{
  if (out->bpp == IMAGE_BPP_BINARY) {
    if (!mask && !invert && !zero) {
      mve_imlib_binary_grayscale_out_binary_fast(out, img, threshold);
    } else {
      mve_imlib_binary_grayscale_out_binary(out, img, threshold, invert, zero, mask);
    }
  } else {
    if (!mask && !invert && !zero) {
      mve_imlib_binary_grayscale_out_grayscale_fast(out, img, threshold);
    } else {
      mve_imlib_binary_grayscale_out_grayscale(out, img, threshold, invert, zero, mask);
    }
  }
}

/* RGB888 to binary */
/***********************/
static inline void mve_imlib_binary_rgb888_out_binary_fast(image_t *out,
                                                           image_t *img,
                                                           color_thresholds_list_lnk_data_t *threshold)
{
  uint8x16_t u8x16_inc = vidupq_n_u8(0, 1);
  u8x16_inc *= 3;
  uint8x16_t u8x16_l;
  int8x16_t s8x16_a, s8x16_b;
  /* Init val */
  u8x16_l = vdupq_n_u8(0);
  s8x16_a = vdupq_n_s8(0);
  s8x16_b = vdupq_n_s8(0);
  for (int y = 0; y < img->h; y++) {
    rgb888_t *old_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = (uint16_t *)IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p = vctp8q(x);
      uint8x16_t u8x16_r, u8x16_g, u8x16_b;
      uint16x8_t u16x8_data_b, u16x8_data_t;
      mve_pred16_t pred;
      /* Load RGB */
      u8x16_r = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->r), u8x16_inc, p);
      u8x16_g = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->g), u8x16_inc, p);
      u8x16_b = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->b), u8x16_inc, p);
      _mve_rgb888_u8x16_to_2_u16x8(u8x16_r, u8x16_g, u8x16_b, &u16x8_data_b, &u16x8_data_t);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab(u16x8_data_b, u16x8_data_t, &u8x16_l, &s8x16_a, &s8x16_b);
      pred = _mve_color_threshold_lab(u8x16_l, s8x16_a, s8x16_b, threshold);
      *out_row_ptr = pred;
      out_row_ptr ++;
      old_row_ptr += 16;
      x -= 16;
    }
  }
}

static inline void mve_imlib_binary_rgb888_out_binary(image_t *out,
                                                      image_t *img,
                                                      color_thresholds_list_lnk_data_t *threshold,
                                                      bool invert,
                                                      bool zero,
                                                      image_t *mask)
{
  uint8x16_t u8x16_inc = vidupq_n_u8(0, 1);
  u8x16_inc *= 3;
  uint8x16_t u8x16_l;
  int8x16_t s8x16_a, s8x16_b;
  /* Init val */
  u8x16_l = vdupq_n_u8(0);
  s8x16_a = vdupq_n_s8(0);
  s8x16_b = vdupq_n_s8(0);
  for (int y = 0; y < img->h; y++) {
    rgb888_t *old_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = (uint16_t *)IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p = vctp8q(x);
      mve_pred16_t u8x16_out;
      uint8x16_t u8x16_r, u8x16_g, u8x16_b;
      uint16x8_t u16x8_data_b, u16x8_data_t;
      mve_pred16_t pred, p_mask;
      p_mask = (mask != NULL) ? mve_image_get_mask_pixel(mask, x, y) : 0xFFFF;
      /* Load RGB */
      u8x16_r = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->r), u8x16_inc, p);
      u8x16_g = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->g), u8x16_inc, p);
      u8x16_b = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->b), u8x16_inc, p);
      _mve_rgb888_u8x16_to_2_u16x8(u8x16_r, u8x16_g, u8x16_b, &u16x8_data_b, &u16x8_data_t);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab(u16x8_data_b, u16x8_data_t, &u8x16_l, &s8x16_a, &s8x16_b);
      u8x16_out = _mve_rgb888_to_binary(u8x16_r, u8x16_g, u8x16_b);
      pred = _mve_color_threshold_lab(u8x16_l, s8x16_a, s8x16_b, threshold);
      if (invert) {
        pred ^= 0xFFFF;
      }
      /* Build out value */
      if (zero) {
        pred &= p_mask;
        u8x16_out &= ~pred;
      } else {
        u8x16_out = (u8x16_out & ~p_mask) | (pred & p_mask);
      }
      *out_row_ptr++ = u8x16_out; /* 16bits each time */
      old_row_ptr += 16;
      x -= 16;
    }
  }
}

static inline void mve_imlib_binary_rgb888_out_rgb888_fast(image_t *out,
                                                           image_t *img,
                                                           color_thresholds_list_lnk_data_t *threshold)
{
  uint8x16_t u8x16_inc = vidupq_n_u8(0, 1);
  u8x16_inc *= 3;
  const rgb888_t out1 = COLOR_YUV_TO_RGB888(127, 0, 0);
  const rgb888_t out0 = COLOR_YUV_TO_RGB888(-128, 0, 0);
  uint8x16_t u8x16_l;
  int8x16_t s8x16_a, s8x16_b;
  /* Init val */
  u8x16_l = vdupq_n_u8(0);
  s8x16_a = vdupq_n_s8(0);
  s8x16_b = vdupq_n_s8(0);
  for (int y = 0; y < img->h; y++) {
    rgb888_t *old_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
    rgb888_t *out_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p = vctp8q(x);
      uint8x16_t u8x16_out;
      uint8x16_t u8x16_r, u8x16_g, u8x16_b;
      uint16x8_t u16x8_data_b, u16x8_data_t;
      mve_pred16_t pred;
      /* Load RGB */
      u8x16_r = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->r), u8x16_inc, p);
      u8x16_g = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->g), u8x16_inc, p);
      u8x16_b = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->b), u8x16_inc, p);
      _mve_rgb888_u8x16_to_2_u16x8(u8x16_r, u8x16_g, u8x16_b, &u16x8_data_b, &u16x8_data_t);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab(u16x8_data_b, u16x8_data_t, &u8x16_l, &s8x16_a, &s8x16_b);
      pred = _mve_color_threshold_lab(u8x16_l, s8x16_a, s8x16_b, threshold);
      /* Build out value */
      u8x16_out = vpselq_u8(
                  vdupq_n_u8(out1.r),
                  vdupq_n_u8(out0.r),
                  pred);
      vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->r), u8x16_inc, u8x16_out, p);
      u8x16_out = vpselq_u8(
                  vdupq_n_u8(out1.g),
                  vdupq_n_u8(out0.g),
                  pred);
      vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->g), u8x16_inc, u8x16_out, p);
      u8x16_out = vpselq_u8(
                  vdupq_n_u8(out1.b),
                  vdupq_n_u8(out0.b),
                  pred);
      vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->b), u8x16_inc, u8x16_out, p);

      out_row_ptr += 16;
      old_row_ptr += 16;
      x -= 16;
    }
  }
}

static inline void mve_imlib_binary_rgb888_out_rgb888(image_t *out,
                                                      image_t *img,
                                                      color_thresholds_list_lnk_data_t *threshold,
                                                      bool invert,
                                                      bool zero,
                                                      image_t *mask)
{
  uint8x16_t u8x16_inc = vidupq_n_u8(0, 1);
  u8x16_inc *= 3;
  const rgb888_t out1 = COLOR_YUV_TO_RGB888(127, 0, 0);
  const rgb888_t out0 = COLOR_YUV_TO_RGB888(-128, 0, 0);
  uint8x16_t u8x16_l;
  int8x16_t s8x16_a, s8x16_b;
  /* Init val */
  u8x16_l = vdupq_n_u8(0);
  s8x16_a = vdupq_n_s8(0);
  s8x16_b = vdupq_n_s8(0);
  for (int y = 0; y < img->h; y++) {
    rgb888_t *old_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
    rgb888_t *out_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p = vctp8q(x);
      uint8x16_t u8x16_out;
      uint8x16_t u8x16_r, u8x16_g, u8x16_b;
      uint16x8_t u16x8_data_b, u16x8_data_t;
      mve_pred16_t pred, p_mask;
      p_mask = (mask != NULL) ? mve_image_get_mask_pixel(mask, x, y) : 0xFFFF;
      /* Load RGB */
      u8x16_r = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->r), u8x16_inc, p);
      u8x16_g = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->g), u8x16_inc, p);
      u8x16_b = vldrbq_gather_offset_z_u8((uint8_t *)&((old_row_ptr)->b), u8x16_inc, p);
      _mve_rgb888_u8x16_to_2_u16x8(u8x16_r, u8x16_g, u8x16_b, &u16x8_data_b, &u16x8_data_t);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab(u16x8_data_b, u16x8_data_t, &u8x16_l, &s8x16_a, &s8x16_b);
      pred = _mve_color_threshold_lab(u8x16_l, s8x16_a, s8x16_b, threshold);
      if (invert) {
        pred ^= 0xFFFF;
      }
      pred &= p_mask;
      /* Build out value */
      if (zero) {
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(out0.r),
                        u8x16_r,
                        pred);
        vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->r), u8x16_inc, u8x16_out, p);
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(out0.g),
                        u8x16_g,
                        pred);
        vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->g), u8x16_inc, u8x16_out, p);
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(out0.b),
                        u8x16_b,
                        pred);
        vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->b), u8x16_inc, u8x16_out, p);
      } else {
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(out1.r),
                        vdupq_n_u8(out0.r),
                        pred);
        vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->r), u8x16_inc, u8x16_out, p);
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(out1.g),
                        vdupq_n_u8(out0.g),
                        pred);
        vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->g), u8x16_inc, u8x16_out, p);
        u8x16_out = vpselq_u8(
                        vdupq_n_u8(out1.b),
                        vdupq_n_u8(out0.b),
                        pred);
        vstrbq_scatter_offset_p_u8((uint8_t *)&((out_row_ptr)->b), u8x16_inc, u8x16_out, p);
      }
      out_row_ptr += 16;
      old_row_ptr += 16;
      x -= 16;
    }
  }
}

void mve_imlib_binary_rgb888(image_t *out,
                                image_t *img,
                                color_thresholds_list_lnk_data_t *threshold,
                                bool invert,
                                bool zero,
                                image_t *mask)
{
  if (out->bpp == IMAGE_BPP_BINARY) {
    if (!mask && !invert && !zero) {
      mve_imlib_binary_rgb888_out_binary_fast(out, img, threshold);
    } else {
      mve_imlib_binary_rgb888_out_binary(out, img, threshold, invert, zero, mask);
    }
  } else {
    if (!mask && !invert && !zero) {
      mve_imlib_binary_rgb888_out_rgb888_fast(out, img, threshold);
    } else {
      mve_imlib_binary_rgb888_out_rgb888(out, img, threshold, invert, zero, mask);
    }
  }
}

/* RGB565 to binary */
/***********************/
static inline void mve_imlib_binary_rgb565_out_binary_fast(image_t *out,
                                                           image_t *img,
                                                           color_thresholds_list_lnk_data_t *threshold)
{
  uint16x8_t u16x8_inc = vidupq_n_u16(0, 1);
  u16x8_inc *= 2 * sizeof(uint16_t);
  uint8x16_t u8x16_l;
  int8x16_t s8x16_a, s8x16_b;
  /* Init val */
  u8x16_l = vdupq_n_u8(0);
  s8x16_a = vdupq_n_s8(0);
  s8x16_b = vdupq_n_s8(0);
  for (int y = 0; y < img->h; y++) {
    uint16_t *old_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = (uint16_t *)IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p_b = vctp16q((x+1)>>1);
      mve_pred16_t p_t = vctp16q(x>>1);
      uint16x8_t u16x8_data_b, u16x8_data_t;
      mve_pred16_t pred;
      /* Load RGB */
      u16x8_data_b = vldrhq_gather_offset_z_u16(old_row_ptr + 0, u16x8_inc, p_b);
      u16x8_data_t = vldrhq_gather_offset_z_u16(old_row_ptr + 1, u16x8_inc, p_t);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab(u16x8_data_b, u16x8_data_t, &u8x16_l, &s8x16_a, &s8x16_b);
      pred = _mve_color_threshold_lab(u8x16_l, s8x16_a, s8x16_b, threshold);
      *out_row_ptr++ = pred; /* 16bits each time */
      old_row_ptr += 16;
      x -= 16;
    }
  }
}

static inline void mve_imlib_binary_rgb565_out_binary(image_t *out,
                                                      image_t *img,
                                                      color_thresholds_list_lnk_data_t *threshold,
                                                      bool invert,
                                                      bool zero,
                                                      image_t *mask)
{
  uint16x8_t u16x8_inc = vidupq_n_u16(0, 1);
  u16x8_inc *= 2 * sizeof(uint16_t);
  uint8x16_t u8x16_l;
  int8x16_t s8x16_a, s8x16_b;
  /* Init val */
  u8x16_l = vdupq_n_u8(0);
  s8x16_a = vdupq_n_s8(0);
  s8x16_b = vdupq_n_s8(0);
  for (int y = 0; y < img->h; y++) {
    uint16_t *old_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = (uint16_t *)IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p_b = vctp16q((x+1)>>1);
      mve_pred16_t p_t = vctp16q(x>>1);
      uint16_t u16_out;
      uint16x8_t u16x8_data_b, u16x8_data_t;
      mve_pred16_t pred, p_mask;
      p_mask = (mask != NULL) ? mve_image_get_mask_pixel(mask, x, y) : 0xFFFF;
      /* Load RGB */
      u16x8_data_b = vldrhq_gather_offset_z_u16(old_row_ptr + 0, u16x8_inc, p_b);
      u16x8_data_t = vldrhq_gather_offset_z_u16(old_row_ptr + 1, u16x8_inc, p_t);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab(u16x8_data_b, u16x8_data_t, &u8x16_l, &s8x16_a, &s8x16_b);
      u16_out = _mve_rgb565_to_binary(u16x8_data_b, u16x8_data_t);
      pred = _mve_color_threshold_lab(u8x16_l, s8x16_a, s8x16_b, threshold);
      if (invert) {
        pred ^= 0xFFFF;
      }
      /* Build out value */
      if (zero) {
        pred &= p_mask;
        u16_out &= ~pred;
      } else {
        u16_out = (u16_out & ~p_mask) | (pred & p_mask);
      }
      *out_row_ptr++ = u16_out; /* 16bits each time */
      old_row_ptr += 16;
      x -= 16;
    }
  }
}

static inline void mve_imlib_binary_rgb565_out_rgb565_fast(image_t *out,
                                                           image_t *img,
                                                           color_thresholds_list_lnk_data_t *threshold)
{
  uint16x8_t u16x8_inc = vidupq_n_u16(0, 1);
  u16x8_inc *= 2;
  const uint16_t out1 = COLOR_YUV_TO_RGB565(127, 0, 0);
  const uint16_t out0 = COLOR_YUV_TO_RGB565(-128, 0, 0);
  uint16x8_t u16x8_l;
  int16x8_t s16x8_a, s16x8_b;
  /* Init val */
  u16x8_l = vdupq_n_u16(0);
  s16x8_a = vdupq_n_s16(0);
  s16x8_b = vdupq_n_s16(0);
  for (int y = 0; y < img->h; y++) {
    uint16_t *old_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p = vctp16q(x);
      uint16x8_t u16x8_out;
      uint16x8_t u16x8_data;
      mve_pred16_t pred;
      /* Load RGB */
      u16x8_data = vldrhq_gather_offset_z_u16(old_row_ptr, u16x8_inc, p);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab16(u16x8_data, &u16x8_l, &s16x8_a, &s16x8_b);
      pred = __mve_color_threshold_lab16(u16x8_l, s16x8_a, s16x8_b, threshold);
      /* Build out value */
      u16x8_out = vpselq_u16(
                  vdupq_n_u16(out1),
                  vdupq_n_u16(out0),
                  pred);
      vstrhq_scatter_offset_p_u16(out_row_ptr, u16x8_inc, u16x8_out, p);
      out_row_ptr += 8;
      old_row_ptr += 8;
      x -= 8;
    }
  }
}

static inline void mve_imlib_binary_rgb565_out_rgb565(image_t *out,
                                                      image_t *img,
                                                      color_thresholds_list_lnk_data_t *threshold,
                                                      bool invert,
                                                      bool zero,
                                                      image_t *mask)
{
  uint16x8_t u16x8_inc = vidupq_n_u16(0, 1);
  u16x8_inc *= 2;
  const uint16_t out1 = COLOR_YUV_TO_RGB565(127, 0, 0);
  const uint16_t out0 = COLOR_YUV_TO_RGB565(-128, 0, 0);
  uint16x8_t u16x8_l;
  int16x8_t s16x8_a, s16x8_b;
  /* Init val */
  u16x8_l = vdupq_n_u16(0);
  s16x8_a = vdupq_n_s16(0);
  s16x8_b = vdupq_n_s16(0);
  for (int y = 0; y < img->h; y++) {
    uint16_t *old_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
    uint16_t *out_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(out, y);
    int x = img->w;
    while (x > 0) {
      mve_pred16_t p = vctp16q(x);
      uint16x8_t u16x8_out;
      uint16x8_t u16x8_data;
      mve_pred16_t pred;
      /* Load RGB */
      u16x8_data = vldrhq_gather_offset_z_u16(old_row_ptr, u16x8_inc, p);
      /* Convert to LAB + Threshold comparison*/
      _mve_color_rgb565_to_lab16(u16x8_data, &u16x8_l, &s16x8_a, &s16x8_b);
      pred = __mve_color_threshold_lab16(u16x8_l, s16x8_a, s16x8_b, threshold);
      if (invert) {
        pred ^= 0xFFFF;
      }
      if (zero) {
          u16x8_out = vpselq_u16(
                      vdupq_n_u16(out0),
                      u16x8_data,
                      pred);
          vstrhq_scatter_offset_p_u16(out_row_ptr, u16x8_inc, u16x8_out, p);
      } else {
          u16x8_out = vpselq_u16(
                      vdupq_n_u16(out1),
                      vdupq_n_u16(out0),
                      pred);
          vstrhq_scatter_offset_p_u16(out_row_ptr, u16x8_inc, u16x8_out, p);
      }
      /* Build out value */
      out_row_ptr += 8;
      old_row_ptr += 8;
      x -= 8;
    }
  }
}

void mve_imlib_binary_rgb565(image_t *out,
                                image_t *img,
                                color_thresholds_list_lnk_data_t *threshold,
                                bool invert,
                                bool zero,
                                image_t *mask)
{
  if (out->bpp == IMAGE_BPP_BINARY) {
    if (!mask && !invert && !zero) {
      mve_imlib_binary_rgb565_out_binary_fast(out, img, threshold);
    } else {
      mve_imlib_binary_rgb565_out_binary(out, img, threshold, invert, zero, mask);
    }
  } else {
    if (!mask && !invert && !zero) {
        mve_imlib_binary_rgb565_out_rgb565_fast(out, img, threshold);
    } else {
      mve_imlib_binary_rgb565_out_rgb565(out, img, threshold, invert, zero, mask);
    }
  }
}
int mve_imlib_erode_dilate_grayscale(image_t *img,
                                     int ksize,
                                     int threshold,
                                     int e_or_d,
                                     image_t *mask)
{
  int brows = ksize + 1;
  image_t buf;
  buf.w = img->w;
  buf.h = brows;
  buf.bpp = img->bpp;
  mve_pred16_t p_r = vctp16q(2 * ksize + 1);
  buf.data = fb_alloc(IMAGE_GRAYSCALE_LINE_LEN_BYTES(img) * brows, FB_ALLOC_NO_HINT);
  if (!buf.data) {
    return -1;
  }

  for (int y = 0; y < img->h; y++) {
    uint16x8_t u16x8_vOffset = {0,};
    uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
    uint8_t *buf_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&buf, (y % brows));
    /* vertical offset to read */
    for (int j = 0; j < 2 * ksize + 1; j++) {
      int compute = IM_MIN(IM_MAX(y - ksize + j, 0), (img->h - 1));
      u16x8_vOffset[j] = (uint16_t) compute;
    }
    u16x8_vOffset -= (uint16_t) IM_MAX(y - ksize, 0);
    u16x8_vOffset *= (uint16_t) img->w;

    int acc = 0;
    uint8_t *k_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, IM_MAX(y - ksize, 0));
    uint16x8_t u16x8_read;
    for (int x = 0; x < img->w; x++) {
      IMAGE_PUT_GRAYSCALE_PIXEL_FAST(buf_row_ptr, x, IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x));

      if (mask && (!image_get_mask_pixel(mask, x, y))) {
        continue;
      }

      if (x > ksize && x < img->w - ksize) { /* faster */
        /* Substract left column */
        u16x8_read = vldrbq_gather_offset_z_u16(k_row_ptr + x - ksize - 1,
                                                u16x8_vOffset,
                                                p_r);
        acc -= vaddvaq_u16(0, vandq_u16(u16x8_read, vdupq_n_u16(1)));
        /* Add right column */
        u16x8_read = vldrbq_gather_offset_z_u16(k_row_ptr + x + ksize,
                                                u16x8_vOffset,
                                                p_r);
        acc = vaddvaq_u16(acc, vandq_u16(u16x8_read, vdupq_n_u16(1)));
      } else { /* slower way which checks boundaries per pixel */
        acc = e_or_d ? 0 : -1; /* Don't count center pixel... */

        /* Adding columns */
        for (int k = -ksize; k < ksize + 1; k++) {
          u16x8_read = vldrbq_gather_offset_z_u16(k_row_ptr + IM_MIN(IM_MAX(x + k, 0), (img->w - 1)),
                                                  u16x8_vOffset,
                                                  p_r);
            acc = vaddvaq_u16(acc, vandq_u16(u16x8_read, vdupq_n_u16(1)));
        }  /* for k */
      }
      if (!e_or_d) {
        /* Preserve original pixel value... or clear it. */
        if (acc < threshold)
          IMAGE_PUT_GRAYSCALE_PIXEL_FAST(buf_row_ptr, x, COLOR_GRAYSCALE_BINARY_MIN);
      } else {
        /* Preserve original pixel value... or set it. */
        if (acc > threshold)
          IMAGE_PUT_GRAYSCALE_PIXEL_FAST(buf_row_ptr, x, COLOR_GRAYSCALE_BINARY_MAX);
      }
    }

    if (y >= ksize) { /* Transfer buffer lines... */
      memcpy(IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, (y - ksize)), 
             IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&buf, ((y - ksize) % brows)),
             IMAGE_GRAYSCALE_LINE_LEN_BYTES(img));
    }
  }

  /* Copy any remaining lines from the buffer image... */
  for (int y = IM_MAX(img->h - ksize, 0), yy = img->h; y < yy; y++) {
    memcpy(IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y),
           IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&buf, (y % brows)),
           IMAGE_GRAYSCALE_LINE_LEN_BYTES(img));
  }
  fb_free();
  return 1;
}
#endif /* IPL_BINARY_HAS_MVE */
