/**
 ******************************************************************************
 * @file   stm32ipl.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library functions header file
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

#ifndef __STM32IPL_H_
#define __STM32IPL_H_

#include "stm32ipl_conf.h"
#include "imlib.h"

typedef enum _stm32ipl_err_t
{
  stm32ipl_err_Ok                 =  0,
  stm32ipl_err_Generic            = -1,
  stm32ipl_err_InvalidParameter   = -2,
  stm32ipl_err_OutOfMemory        = -3,
  stm32ipl_err_BadPointer         = -4,
  stm32ipl_err_UnsupportedFormat  = -5,
  stm32ipl_err_OpeningFile        = -6,
  stm32ipl_err_ClosingFile        = -7,
  stm32ipl_err_ReadingFile        = -8,
  stm32ipl_err_WritingFile        = -9,
  stm32ipl_err_SeekingFile        = -10,
  stm32ipl_err_NotImplemented     = -11,
  stm32ipl_err_OpNotCompleted     = -12,
  stm32ipl_err_WrongSize          = -13,
  stm32ipl_err_EmptyImage         = -14,
  stm32ipl_err_EmptyMatrix        = -15,
  stm32ipl_err_WrongMatrixDim     = -16,
  stm32ipl_err_ZeroMatrixDim      = -17,
  stm32ipl_err_ReadingDatabase    = -18,
  stm32ipl_err_WritingDatabase    = -19,
  stm32ipl_err_UnsupportedMethod  = -20,
  stm32ipl_err_NotAllowed         = -21,
  stm32ipl_err_NotInPlaceFunction = -22,
  stm32ipl_err_OpeningSource      = -23,
  stm32ipl_err_WrongROI           = -24,
} stm32ipl_err_t;


/* Init library. */
void STM32Ipl_Init(uint32_t mem_address, uint32_t mem_size);

/*  Uninit library. */
void STM32Ipl_Uninit(void);

/* Allocate the data memory of the image. */
stm32ipl_err_t STM32Ipl_AllocateData(image_t *img, uint32_t width, uint32_t height, image_bpp_t bpp);

/* Release the data memory of the image. */
void STM32Ipl_ReleaseData(image_t * img);

/* Copy the image structure only. */
stm32ipl_err_t STM32Ipl_Copy(const image_t *src, image_t *dst);

/* Copy the image data only. */
stm32ipl_err_t STM32Ipl_CopyData(const image_t *src, image_t *dst);

/* Clone the image. */
stm32ipl_err_t STM32Ipl_Clone(const image_t *src, image_t *dst);

/* Get the size of the image data (bytes). */
uint32_t STM32Ipl_ImageDataSize(const image_t *img);

/* Get the size of the memory needed to store an image with the given properties. */
uint32_t STM32Ipl_DataSize(uint32_t width, uint32_t height, image_bpp_t bpp);

stm32ipl_err_t STM32Ipl_Invert(image_t *img);

/* Histogram equalization. */
stm32ipl_err_t STM32Ipl_HistEq(image_t *img, const image_t *mask);
stm32ipl_err_t STM32Ipl_ClaheHistEq(image_t *img, float clip_limit, image_t *mask);

/* Filtering. */
stm32ipl_err_t STM32Ipl_MeanFilter(image_t *img, const int32_t ksize, bool threshold, int32_t offset, bool invert, image_t *mask);
stm32ipl_err_t STM32Ipl_MedianFilter(image_t *img, int ksize, float percentile, bool threshold, int offset, bool invert, image_t *mask);

// Object detector.
stm32ipl_err_t STM32Ipl_LoadCascadeFromMemory(cascade_t *cascade, const uint8_t *memory);
stm32ipl_err_t STM32Ipl_LoadFaceCascade(cascade_t *cascade);
stm32ipl_err_t STM32Ipl_LoadEyeCascade(cascade_t *cascade);
array_t * STM32Ipl_DetectObject(cascade_t *cascade, image_t* img, rectangle_t *roi, float scale_factor, float threshold);


// Binary Function.
stm32ipl_err_t STM32Ipl_Binary(image_t *dst_img, image_t *src_img, list_t *thresholds, bool invert, bool zero, image_t *mask);


// Rotation.
stm32ipl_err_t STM32Ipl_Rotation(image_t *img, float x_rotation, float y_rotation,
                         float z_rotation, float x_translation, float y_translation,
                         float zoom, float fov, float *corners);


// Color space conversion
stm32ipl_err_t STM32Ipl_Convert(image_t * src_img, image_t * dst_img);


// Crop and scale.
stm32ipl_err_t STM32Ipl_Crop(image_t * src_img, image_t * dst_img, rectangle_t* roi);
stm32ipl_err_t STM32Ipl_Scale(image_t * src_img, image_t * dst_img, rectangle_t* roi, float x_scale, float y_scale);

stm32ipl_err_t STM32Ipl_Replace(image_t *src_img, image_t *dst_img, bool hmirror, bool vflip, bool transpose,
		image_t *mask);

// Flip, mirror and 90°, 180° and 270° rotations.
stm32ipl_err_t STM32Ipl_Flip(image_t * src_img, image_t* dst_img);
stm32ipl_err_t STM32Ipl_Mirror(image_t * src_img, image_t * dst_img);
stm32ipl_err_t STM32Ipl_Rotation90(image_t * src_img, image_t * dst_img);
stm32ipl_err_t STM32Ipl_Rotation180(image_t * src_img, image_t * dst_img);
stm32ipl_err_t STM32Ipl_Rotation270(image_t * src_img, image_t * dst_img);
stm32ipl_err_t STM32Ipl_FlipMirror(image_t * src_img, image_t * dst_img);


// Gaussian filter.
stm32ipl_err_t STM32Ipl_Gaussian(image_t *img, uint8_t ksize, bool threshold, bool unsharp);

// Laplacian filter.
stm32ipl_err_t STM32Ipl_Laplacian(image_t *img, uint8_t ksize, bool sharpen);

// Sobel filter.
stm32ipl_err_t STM32Ipl_Sobel(image_t *img, uint8_t ksize, bool sharpen);

// Scharr filter.
stm32ipl_err_t STM32Ipl_Scharr(image_t *img, uint8_t ksize, bool sharpen);

// Kernel filter.
stm32ipl_err_t STM32Ipl_Morph(image_t *img, int ksize, int *krn, float mul, /* CMARCH float*/ int add, bool threshold, int offset, bool invert, image_t *mask);

// Canny edge filter.
stm32ipl_err_t STM32Ipl_EdgeCanny(image_t * grayscale_img, uint8_t min_threshold, uint8_t max_threshold);

// Hough edge circle filter.
stm32ipl_err_t STM32Ipl_FindCircles(image_t* img, rectangle_t* roi, list_t* out, uint32_t x_stride,
		                            uint32_t y_stride, uint32_t threshold, uint32_t x_margin, uint32_t y_margin,
									uint32_t r_margin, uint32_t r_min, uint32_t r_max, uint32_t r_step);

// Morphological operators.
stm32ipl_err_t STM32Ipl_Dilate(image_t* img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_Erode(image_t* img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_Open(image_t* img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_Close(image_t* img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_TopHat(image_t *img, uint8_t ksize, uint8_t threshold, image_t *mask);
stm32ipl_err_t STM32Ipl_BlackHat(image_t *img, uint8_t ksize, uint8_t threshold, image_t *mask);

//Boolean.
stm32ipl_err_t STM32Ipl_B_and(image_t* imgA, image_t* imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_nand(image_t* imgA, image_t* imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_or(image_t* imgA, image_t* imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_nor(image_t* imgA, image_t* imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_xor(image_t* imgA, image_t* imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_xnor(image_t* imgA, image_t* imgB, image_t *mask);

// Histogram
stm32ipl_err_t STM32Ipl_GetHistogram(image_t *img, histogram_t* hist, rectangle_t* roi);

//Statistics
stm32ipl_err_t STM32Ipl_GetStatistics(image_t *img, statistics_t* stats, rectangle_t* roi);

//Gamma Correction
stm32ipl_err_t STM32Ipl_GammaCorr(image_t *img, float gamma_val, float contrast, float brightness);

#endif  // __STM32IPL_H_
