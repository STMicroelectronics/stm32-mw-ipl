/**
 ******************************************************************************
 * @file    mve_filter.c
 * @author  AIS Team
 * @brief   MVE Image processing library filter functions with MVE intrinsics

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

#ifdef IPL_FILTER_HAS_MVE
#include "mve_filter.h"
#include "filter_int.h"
 

int mve_imlib_median_filter_grayscale(image_t *img,
                                      const int ksize,
                                      float percentile,
                                      bool threshold,
                                      int offset,
                                      bool invert,
                                      image_t *mask)
{
  int brows = ksize + 1;
  image_t buf;
  buf.w = img->w;
  buf.h = brows;
  buf.bpp = img->bpp;

  mve_pred16_t p_r = vctp16q(2 * ksize + 1);

  const int n = ((ksize * 2) + 1) * ((ksize * 2) + 1);
  const int median_cutoff = fast_floorf(percentile * (float) n);
  buf.data = fb_alloc(IMAGE_GRAYSCALE_LINE_LEN_BYTES(img) * brows, FB_ALLOC_NO_HINT);
  uint8_t *data = fb_alloc(64, FB_ALLOC_NO_HINT);

  for (int y = 0, yy = img->h; y < yy; y++) {
    uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
    uint8_t *buf_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&buf, (y % brows));

    uint16x8_t u16x8_vOffset = {0,};
    /* vertical offset to read */
    for (int j = 0; j < 2 * ksize + 1; j++) {
      int compute = IM_MIN(IM_MAX(y - ksize + j, 0), (img->h - 1));
      u16x8_vOffset[j] = (uint16_t) compute;
    }
    u16x8_vOffset -= (uint16_t) IM_MAX(y - ksize, 0);
    u16x8_vOffset *= (uint16_t) img->w;

    uint8_t *k_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, IM_MAX(y - ksize, 0));
    for (int x = 0, xx = img->w; x < xx; x++) {
      if (mask && (!image_get_mask_pixel(mask, x, y))) {
        IMAGE_PUT_GRAYSCALE_PIXEL_FAST(buf_row_ptr, x, IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x));
        continue; /* Short circuit. */
      }

      if (!mask && x > ksize && x < img->w - ksize) { /* faster */
        uint16x8_t u16x8_pixel;
        u16x8_pixel = vldrbq_gather_offset_z_u16(k_row_ptr + x - ksize - 1, u16x8_vOffset, p_r);
        u16x8_pixel = vshrq_n_u16(u16x8_pixel, 2);
        for (int _idx = 0; _idx < 2 * ksize + 1; _idx++) {
          data[u16x8_pixel[_idx]]--;
        }
        u16x8_pixel = vldrbq_gather_offset_z_u16(k_row_ptr + x + ksize, u16x8_vOffset, p_r);
        u16x8_pixel = vshrq_n_u16(u16x8_pixel, 2);
        for (int _idx = 0; _idx < 2 * ksize + 1; _idx++) {
          data[u16x8_pixel[_idx]]++;
        }
      } else { /* slower way which checks boundaries per pixel */
        memset(data, 0, 64);

        for (int k = -ksize; k <= ksize; k++) {
          uint16x8_t u16x8_pixel;
          u16x8_pixel = vldrbq_gather_offset_z_u16(
                             k_row_ptr + IM_MIN(IM_MAX(x + k, 0), (img->w - 1)),
                             u16x8_vOffset, p_r);

          u16x8_pixel = vshrq_n_u16(u16x8_pixel, 2);
          for (int _idx = 0; _idx < 2 * ksize + 1; _idx++) {
            data[u16x8_pixel[_idx]]++;
          }
        }
      }
      uint8_t pixel;
      pixel = hist_median(data, 64, median_cutoff); /* find the median */
      pixel <<= 2; /* scale it back up */
      if (threshold) {
        if (((pixel - offset) < IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x)) ^ invert) {
          pixel = COLOR_GRAYSCALE_BINARY_MAX;
        } else {
          pixel = COLOR_GRAYSCALE_BINARY_MIN;
        }
      }

      IMAGE_PUT_GRAYSCALE_PIXEL_FAST(buf_row_ptr, x, pixel);
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
  fb_free(); /* buf.data */
  return 0;
}
int mve_imlib_morph_u8(image_t *img,
                       const int ksize,
                       const uint8_t *krn,
                       const float m,
                       const int b,
                       bool threshold,
                       int offset,
                       bool invert,
                       image_t *mask)
{
  int brows = ksize + 1;
  image_t buf;
  buf.w = img->w;
  buf.h = brows;
  buf.bpp = img->bpp;
  const int32_t m_int = (int32_t) (65536.0f * m); /* m is 1/kernel_weight    ## STM32IPL: f added to the constant. */

  int rer_val = -1;
  if ((2 * ksize + 1) > 16) {
    return rer_val;
  }
  if (!krn) {
    return rer_val;
  }

  switch (img->bpp) {
    case IMAGE_BPP_BINARY: {
      /* not supported */
      break;
    }
    case IMAGE_BPP_GRAYSCALE: {

      buf.data = fb_alloc(IMAGE_GRAYSCALE_LINE_LEN_BYTES(img) * brows, FB_ALLOC_NO_HINT);
      for (int y = 0, yy = img->h; y < yy; y++) {
        uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
        uint8_t *buf_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&buf, (y % brows));

        for (int x = 0, xx = img->w; x < xx; x++) {
          if (mask && (!image_get_mask_pixel(mask, x, y))) {
            IMAGE_PUT_GRAYSCALE_PIXEL_FAST(buf_row_ptr, x, IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x));
            continue; /* Short circuit. */
          }

          int32_t acc = 0;
          const uint8_t *p_krn_u8 = krn;
          size_t size = 2 * ksize + 1;
          if (x >= ksize && x < img->w - ksize) {
            if (ksize == 1)  {
              uint8x16_t u8x16_krn, u8x16_data;
              u8x16_krn = vldrbq_z_u8(p_krn_u8, vctp8q(size * size));
              uint8_t *k_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, IM_MAX(y - 1, 0));
              u8x16_data[0] = k_row_ptr[x-ksize];
              u8x16_data[1] = k_row_ptr[x-ksize + 1];
              u8x16_data[2] = k_row_ptr[x-ksize + 2];
              k_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
              u8x16_data[3] = k_row_ptr[x-ksize];
              u8x16_data[4] = k_row_ptr[x-ksize + 1];
              u8x16_data[5] = k_row_ptr[x-ksize + 2];
              k_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, IM_MIN(y + 1, img->h - 1));
              u8x16_data[6] = k_row_ptr[x-ksize];
              u8x16_data[7] = k_row_ptr[x-ksize + 1];
              u8x16_data[8] = k_row_ptr[x-ksize + 2];
              acc += vmladavq_u8(u8x16_krn, u8x16_data);
            } else {
              for (int j = -ksize; j <= ksize; j++) {
                /* Load each line */
                uint8_t *k_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, IM_MIN(IM_MAX(y + j, 0), img->h - 1));
                uint8x16_t u8x16_krn, u8x16_data;
                /* Load kernel */
                u8x16_krn = vldrbq_z_u8(p_krn_u8, vctp8q(size));
                p_krn_u8 += size;
                /* Load data */
                u8x16_data = vldrbq_z_u8((uint8_t *)(k_row_ptr + (x - ksize)), vctp8q(size));
                acc += vmladavq_u8(u8x16_krn, u8x16_data);
              }
            }
          } else {
            for (int j = -ksize; j <= ksize; j++) {
              size = 2 * ksize + 1;
              uint8_t *k_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, IM_MIN(IM_MAX(y + j, 0), (img->h - 1)));

              for (int k = x - ksize; k < 0; k++) {
                acc += *p_krn_u8 * k_row_ptr[0];
                size--;
                p_krn_u8++;
              }
              size -= ((x + ksize + 1) > img->w) ? (x + ksize + 1 - img->w) : 0;

              uint8x16_t u8x16_krn, u8x16_data;
              /* Load kernel */
              u8x16_krn = vldrbq_z_u8(p_krn_u8, vctp8q(size));
              p_krn_u8 += size;
              /* Load data */
              u8x16_data = vldrbq_z_u8((uint8_t *)(k_row_ptr + IM_MAX(x - ksize, 0)), vctp8q(size));
              acc += vmladavq_u8(u8x16_krn, u8x16_data);

              /* remaining */
              for (int k = img->w; k <= x + ksize; k++) {
                acc += *p_krn_u8 * k_row_ptr[img->w - 1];
                p_krn_u8++;
              }
            }
          } /* else (x borders) */

          int32_t tmp = (acc * m_int) >> 16;
          int pixel = tmp + b;
          pixel = IM_MIN(IM_MAX(pixel, 0), COLOR_GRAYSCALE_MAX);

          if (threshold) {
            if (((pixel - offset) < IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x)) ^ invert) {
              pixel = COLOR_GRAYSCALE_BINARY_MAX;
            } else {
              pixel = COLOR_GRAYSCALE_BINARY_MIN;
            }
          }

          IMAGE_PUT_GRAYSCALE_PIXEL_FAST(buf_row_ptr, x, pixel);
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

      fb_free(); /* buf.data */
      rer_val = 0; /* correct execution */
      break;
    }
    case IMAGE_BPP_RGB565: {
      /* not supported */
      break;
    }

    case IMAGE_BPP_RGB888: {
      buf.data = fb_alloc(IMAGE_RGB888_LINE_LEN_BYTES(img) * brows, FB_ALLOC_NO_HINT);
      for (int y = 0, yy = img->h; y < yy; y++) {
        rgb888_t *row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
        rgb888_t *buf_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(&buf, (y % brows));

        for (int x = 0, xx = img->w; x < xx; x++) {
          if (mask && (!image_get_mask_pixel(mask, x, y))) {
            IMAGE_PUT_RGB888_PIXEL_FAST(buf_row_ptr, x, IMAGE_GET_RGB888_PIXEL_FAST(row_ptr, x));
            continue; /* Short circuit. */
          }

          int32_t tmp, r_acc = 0, g_acc = 0, b_acc = 0;
          const uint8_t *p_krn_u8 = krn;
          size_t size = 2 * ksize + 1;
          uint8x16_t u8x16_inc = vidupq_n_u8(0, 1);
          u8x16_inc *= 3;

          if (x >= ksize && x < img->w - ksize) {
            if (ksize == 1)  {
              uint8x16_t u8x16_krn, u8x16_data_r, u8x16_data_g, u8x16_data_b;
              u8x16_krn = vldrbq_z_u8(p_krn_u8, vctp8q(size * size));
              rgb888_t *k_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, IM_MAX(y - 1, 0));
              rgb888_t *k_ptr = k_row_ptr + (x - ksize);
              u8x16_data_r[0] = k_ptr[0].r; u8x16_data_g[0] = k_ptr[0].g; u8x16_data_b[0] = k_ptr[0].b;
              u8x16_data_r[1] = k_ptr[1].r; u8x16_data_g[1] = k_ptr[1].g; u8x16_data_b[1] = k_ptr[1].b;
              u8x16_data_r[2] = k_ptr[2].r; u8x16_data_g[2] = k_ptr[2].g; u8x16_data_b[2] = k_ptr[2].b;
              k_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
              k_ptr = k_row_ptr + (x - ksize);
              u8x16_data_r[3] = k_ptr[0].r; u8x16_data_g[3] = k_ptr[0].g; u8x16_data_b[3] = k_ptr[0].b;
              u8x16_data_r[4] = k_ptr[1].r; u8x16_data_g[4] = k_ptr[1].g; u8x16_data_b[4] = k_ptr[1].b;
              u8x16_data_r[5] = k_ptr[2].r; u8x16_data_g[5] = k_ptr[2].g; u8x16_data_b[5] = k_ptr[2].b;
              k_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, IM_MIN(y + 1, img->h - 1));
              k_ptr = k_row_ptr + (x - ksize);
              u8x16_data_r[6] = k_ptr[0].r; u8x16_data_g[6] = k_ptr[0].g; u8x16_data_b[6] = k_ptr[0].b;
              u8x16_data_r[7] = k_ptr[1].r; u8x16_data_g[7] = k_ptr[1].g; u8x16_data_b[7] = k_ptr[1].b;
              u8x16_data_r[8] = k_ptr[2].r; u8x16_data_g[8] = k_ptr[2].g; u8x16_data_b[8] = k_ptr[2].b;
              r_acc += vmladavq_u8(u8x16_krn, u8x16_data_r);
              g_acc += vmladavq_u8(u8x16_krn, u8x16_data_g);
              b_acc += vmladavq_u8(u8x16_krn, u8x16_data_b);
            } else {
              for (int j = -ksize; j <= ksize; j++) {
                /* Load each line */
                rgb888_t *k_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, IM_MIN(IM_MAX(y + j, 0), img->h - 1));
                uint8x16_t u8x16_krn, u8x16_data;
                /* Load kernel */
                u8x16_krn = vldrbq_z_u8(p_krn_u8, vctp8q(size));
                p_krn_u8 += size;
                /* Load data */
                rgb888_t *k_ptr = k_row_ptr + x - ksize;
                u8x16_data = vldrbq_gather_offset_z_u8((uint8_t *)&(k_ptr->r), u8x16_inc, vctp8q(size));
                r_acc += vmladavq_u8(u8x16_krn, u8x16_data);
                /* Load data */
                u8x16_data = vldrbq_gather_offset_z_u8((uint8_t *)&(k_ptr->g), u8x16_inc, vctp8q(size));
                g_acc += vmladavq_u8(u8x16_krn, u8x16_data);
                /* Load data */
                u8x16_data = vldrbq_gather_offset_z_u8((uint8_t *)&(k_ptr->b), u8x16_inc, vctp8q(size));
                b_acc += vmladavq_u8(u8x16_krn, u8x16_data);
              }
            }
          } else {
            for (int j = -ksize; j <= ksize; j++) {
              size = 2 * ksize + 1;
              rgb888_t *k_row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, IM_MIN(IM_MAX(y + j, 0), (img->h - 1)));

              for (int k = x - ksize; k < 0; k++) {
                r_acc += *p_krn_u8 * k_row_ptr[0].r;
                g_acc += *p_krn_u8 * k_row_ptr[0].g;
                b_acc += *p_krn_u8 * k_row_ptr[0].b;
                size--;
                p_krn_u8++;
              }
              size -= ((x + ksize + 1) > img->w) ? (x + ksize + 1 - img->w) : 0;

              uint8x16_t u8x16_krn, u8x16_data;
              /* Load kernel */
              u8x16_krn = vldrbq_z_u8(p_krn_u8, vctp8q(size));
              p_krn_u8 += size;
              /* Load data */
              u8x16_data = vldrbq_gather_offset_z_u8((uint8_t *)&((k_row_ptr + IM_MAX(x - ksize, 0))->r), u8x16_inc, vctp8q(size));
              r_acc += vmladavq_u8(u8x16_krn, u8x16_data);
              u8x16_data = vldrbq_gather_offset_z_u8((uint8_t *)&((k_row_ptr + IM_MAX(x - ksize, 0))->g), u8x16_inc, vctp8q(size));
              g_acc += vmladavq_u8(u8x16_krn, u8x16_data);
              u8x16_data = vldrbq_gather_offset_z_u8((uint8_t *)&((k_row_ptr + IM_MAX(x - ksize, 0))->b), u8x16_inc, vctp8q(size));
              b_acc += vmladavq_u8(u8x16_krn, u8x16_data);

              /* remaining */
              for (int k = img->w; k <= x + ksize; k++) {
                r_acc += *p_krn_u8 * k_row_ptr[img->w - 1].r;
                g_acc += *p_krn_u8 * k_row_ptr[img->w - 1].g;
                b_acc += *p_krn_u8 * k_row_ptr[img->w - 1].b;
                p_krn_u8++;
              }
            }
          } /* else (x borders) */
          tmp = (r_acc * m_int) >> 16;
          r_acc = tmp + b;
          if (r_acc > COLOR_R8_MAX) r_acc = COLOR_R8_MAX;
          else if (r_acc < 0) r_acc = 0;
          tmp = (g_acc * m_int) >> 16;
          g_acc = tmp + b;
          if (g_acc > COLOR_G8_MAX) g_acc = COLOR_G8_MAX;
          else if (g_acc < 0) g_acc = 0;
          tmp = (b_acc * m_int) >> 16;
          b_acc = tmp + b;
          if (b_acc > COLOR_B8_MAX) b_acc = COLOR_B8_MAX;
          else if (b_acc < 0) b_acc = 0;

          rgb888_t pixel888;
          pixel888.r = r_acc;
          pixel888.g = g_acc;
          pixel888.b = b_acc;

          if (threshold) {
              if (((COLOR_RGB888_TO_Y(pixel888.r, pixel888.g, pixel888.b) - offset) < COLOR_RGB888_TO_GRAYSCALE(IMAGE_GET_RGB888_PIXEL_FAST(row_ptr, x))) ^ invert) {
                pixel888.r = COLOR_R8_MAX;
                pixel888.g = COLOR_G8_MAX;
                pixel888.b = COLOR_B8_MAX;
              } else {
                pixel888.r = COLOR_R8_MIN;
                pixel888.g = COLOR_G8_MIN;
                pixel888.b = COLOR_B8_MIN;
              }
          }

          IMAGE_PUT_RGB888_PIXEL_FAST(buf_row_ptr, x, pixel888);
        }

        if (y >= ksize) { /* Transfer buffer lines... */
            memcpy(IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, (y - ksize)),
                   IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(&buf, ((y - ksize) % brows)),
                   IMAGE_RGB888_LINE_LEN_BYTES(img));
        }
      }

      /* Copy any remaining lines from the buffer image... */
      for (int y = IM_MAX(img->h - ksize, 0), yy = img->h; y < yy; y++) {
        memcpy(IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y),
               IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(&buf, (y % brows)),
               IMAGE_RGB888_LINE_LEN_BYTES(img));
      }
      fb_free();
      rer_val = 0; /* correct execution */
      break;
    }

    default: {
      break;
    }
  }

  return rer_val;
}
#endif /* IPL_FILTER_HAS_MVE */
