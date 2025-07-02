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
      mve_pred16_t p = vctp8q(x);
      uint8x16_t u8x16_read = vldrbq_z_u8(old_row_ptr + x, p);
      mve_pred16_t pred, p_mask, data_binary;
      data_binary = vcmpcsq_n_u8(u8x16_read,
                           (((COLOR_GRAYSCALE_MAX - COLOR_GRAYSCALE_MIN) / 2) + COLOR_GRAYSCALE_MIN));
      p_mask = mve_image_get_mask_pixel(mask, x, y);
      pred = vcmpcsq_n_u8(u8x16_read, threshold->LMin); /* u8x16_read >= lnk_data.LMin */
      pred &= vcmpcsq_u8(vdupq_n_u8(threshold->LMax), u8x16_read); /* lnk_data.LMax >= u8x16_read */
      pred ^= invert;
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
      p_mask = mve_image_get_mask_pixel(mask, x, y);
      pred = vcmpcsq_n_u8(u8x16_read, threshold->LMin); /* u8x16_read >= lnk_data.LMin */
      pred = vcmpcsq_m_u8(vdupq_n_u8(threshold->LMax), u8x16_read, pred); /* lnk_data.LMax >= u8x16_read */
      pred ^= invert;
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
