/**
 ******************************************************************************
 * @file   stm32ipl.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library functions header file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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

//#include "stm32ipl_conf.h"
#include "imlib.h"

#define STM32IPL_MAX(a, b) ((a) > (b) ? (a) : (b))
#define STM32IPL_MIN(a, b) ((a) < (b) ? (a) : (b))

///*
// * Each color has the following format: AARRGGBB
// */
//#define STM32IPL_COLOR_BLUE          0xFF0000FFUL
//#define STM32IPL_COLOR_GREEN         0xFF00FF00UL
//#define STM32IPL_COLOR_RED           0xFFFF0000UL
//#define STM32IPL_COLOR_CYAN          0xFF00FFFFUL
//#define STM32IPL_COLOR_MAGENTA       0xFFFF00FFUL
//#define STM32IPL_COLOR_YELLOW        0xFFFFFF00UL
//#define STM32IPL_COLOR_LIGHTBLUE     0xFF8080FFUL
//#define STM32IPL_COLOR_LIGHTGREEN    0xFF80FF80UL
//#define STM32IPL_COLOR_LIGHTRED      0xFFFF8080UL
//#define STM32IPL_COLOR_LIGHTCYAN     0xFF80FFFFUL
//#define STM32IPL_COLOR_LIGHTMAGENTA  0xFFFF80FFUL
//#define STM32IPL_COLOR_LIGHTYELLOW   0xFFFFFF80UL
//#define STM32IPL_COLOR_DARKBLUE      0xFF000080UL
//#define STM32IPL_COLOR_DARKGREEN     0xFF008000UL
//#define STM32IPL_COLOR_DARKRED       0xFF800000UL
//#define STM32IPL_COLOR_DARKCYAN      0xFF008080UL
//#define STM32IPL_COLOR_DARKMAGENTA   0xFF800080UL
//#define STM32IPL_COLOR_DARKYELLOW    0xFF808000UL
//#define STM32IPL_COLOR_WHITE         0xFFFFFFFFUL
//#define STM32IPL_COLOR_LIGHTGRAY     0xFFD3D3D3UL
//#define STM32IPL_COLOR_GRAY          0xFF808080UL
//#define STM32IPL_COLOR_DARKGRAY      0xFF404040UL
//#define STM32IPL_COLOR_BLACK         0xFF000000UL
//#define STM32IPL_COLOR_BROWN         0xFFA52A2AUL
//#define STM32IPL_COLOR_ORANGE        0xFFFFA500UL

//typedef stm32ipl_color_t uint32_t;


typedef enum _stm32ipl_err_t
{
	stm32ipl_err_Ok = 0,
	stm32ipl_err_Generic = -1,
	stm32ipl_err_InvalidParameter = -2,
	stm32ipl_err_OutOfMemory = -3,
	stm32ipl_err_BadPointer = -4,
	stm32ipl_err_UnsupportedFormat = -5,
	stm32ipl_err_OpeningFile = -6,
	stm32ipl_err_ClosingFile = -7,
	stm32ipl_err_ReadingFile = -8,
	stm32ipl_err_WritingFile = -9,
	stm32ipl_err_SeekingFile = -10,
	stm32ipl_err_NotImplemented = -11,
	stm32ipl_err_OpNotCompleted = -12,
	stm32ipl_err_WrongSize = -13,
	stm32ipl_err_EmptyImage = -14,
	stm32ipl_err_EmptyMatrix = -15,
	stm32ipl_err_WrongMatrixDim = -16,
	stm32ipl_err_ZeroMatrixDim = -17,
	stm32ipl_err_ReadingDatabase = -18,
	stm32ipl_err_WritingDatabase = -19,
	stm32ipl_err_UnsupportedMethod = -20,
	stm32ipl_err_NotAllowed = -21,
	stm32ipl_err_NotInPlaceFunction = -22,
	stm32ipl_err_OpeningSource = -23,
	stm32ipl_err_WrongROI = -24,
} stm32ipl_err_t;

/* Initializes a rectangle. */
void STM32Ipl_RectInit(rectangle_t* rect, int32_t x, int32_t y, int32_t width, int32_t height);

/* Copies the source rectangle data to the destination one. */
void STM32Ipl_RectCopy(const rectangle_t* src, rectangle_t* dst);

/* Determines whether the two given rectangles overlap. */
bool STM32Ipl_RectOverlap(const rectangle_t* rect0, const rectangle_t* rect1);

/* Determines whether the second rectangle is inside the first one. */
bool STM32Ipl_RectContain(const rectangle_t* rect0, const rectangle_t* rect1);

/* Gets the union between the two given rectangles, and saves it into rect1. */
void STM32Ipl_RectUnion(const rectangle_t* rect0, rectangle_t* rect1);

/* Gets the intersection between the two given rectangles, and saves it into rect1. */
void STM32Ipl_RectIntersect(const rectangle_t* rect0, rectangle_t* rect1);

/* Initializes the memory manager used by this library. */
void STM32Ipl_InitLib(void *memAddress, uint32_t memSize);

/*  De-initializes the memory manager of this library. */
void STM32Ipl_DeInitLib(void);

/* Initializes the image with the given arguments. */
void STM32Ipl_Init(image_t *img, uint32_t width, uint32_t height, image_bpp_t format, void *data);

///* Resets the whole image structure. */
//void STM32Ipl_Reset(image_t *img);

/* Allocates a data memory buffer to contain the image pixels and consequently initializes the given image structure. */
stm32ipl_err_t STM32Ipl_AllocateData(image_t *img, uint32_t width, uint32_t height, image_bpp_t format);

/* Releases the data memory buffer of the image and resets the image structure. */
void STM32Ipl_ReleaseData(image_t *img);

/* Gets the size of the memory needed to store an image with the given properties.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888). */
uint32_t STM32Ipl_DataSize(uint32_t width, uint32_t height, image_bpp_t format);

/* Gets the size the data buffer of the given image (in bytes).
 * The supported formats are (Binary, Grayscale, RGB565, RGB888). */
uint32_t STM32Ipl_ImageDataSize(const image_t *img);

/* Copies the source image into the destination one. Only the image structure is copied,
 * so beware the source image data will be shared with the destination image, as no new memory
 * buffer is allocated. */
stm32ipl_err_t STM32Ipl_Copy(const image_t *src, image_t *dst);

/* Copies the source image data buffer into the destination one.
 * Only the pixel data is actually copied. Source and destination images must have same resolution and format.
 * The destination image data pointer must refer to a valid memory buffer as no new memory is allocated. */
stm32ipl_err_t STM32Ipl_CopyData(const image_t *src, image_t *dst);

/* Clones the source image into the destination one. If the destination image data pointer
 * is null, a new memory buffer is allocated, filled with the source pixel data and assigned to
 * the destination image. If the destination image data pointer points to a user allocated buffer,
 * such buffer must have the right size to contain the source image. In case of success, the two
 * images will have same resolution and format. */
stm32ipl_err_t STM32Ipl_Clone(const image_t *src, image_t *dst);

/* Sets the image data buffer to zero. */
stm32ipl_err_t STM32Ipl_Zero(image_t *img);

/* Fills the image with the given color. If roi is defined, only the corresponding region is filled. */
stm32ipl_err_t STM32Ipl_Fill(image_t *img, int color, const rectangle_t *roi);

/* Converts the source image data to the format of the destination image and stores the
 * converted data to the destination buffer. The two images must have the same resolution.
 * The destination image data buffer must be already allocated and must have the right size to
 * contain the converted image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888). */
stm32ipl_err_t STM32Ipl_Convert(const image_t *src, image_t *dst);

/* Converts the source image data to the format of the destination image and stores the
 * converted data to the destination buffer. The two images must have the same resolution.
 * The destination image data buffer must be already allocated and must have the right size to
 * contain the converted image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * This function supports some particular cases when source and destination data memory buffers overlap. */
stm32ipl_err_t STM32Ipl_ConvertWithOverlap(const image_t *src, image_t *dst);

/* Resizes the source image to the destination one with Nearest Neighbor method.
 * The two images must have the same format. The destination data buffer must be allocated by the user
 * and its size must be compatible with resolution and format of the destination image.
 * When specified, roi defines the region of the source image to be scaled to the destination image resolution.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888). */
stm32ipl_err_t STM32Ipl_Resize(const image_t *src, image_t *dst, const rectangle_t *roi);

/* Resizes (downscale only) the source image to the destination one with Nearest Neighbor method.
 * The two images must have the same format. The destination data buffer must be allocated by the user
 * and its size must be compatible with resolution and format of the destination image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * This function supports some particular cases when source and destination data memory buffers overlap.
 * Call this function for downscale cases only. */
stm32ipl_err_t STM32Ipl_Downscale(const image_t *src, image_t *dst);

/*******************************************************
 *******************************************************
 * TODO: from this point on, the code must be reviewed.
 *******************************************************
 *******************************************************/
/* Inverts the image. */
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
array_t* STM32Ipl_DetectObject(cascade_t *cascade, const image_t *img, rectangle_t *roi, float scale_factor, float threshold);

// Binary Function.
stm32ipl_err_t STM32Ipl_Binary(image_t *dst, image_t *src, list_t *thresholds, bool invert, bool zero, image_t *mask);

// Rotation.
stm32ipl_err_t STM32Ipl_Rotation(image_t *img, float x_rotation, float y_rotation, float z_rotation,
		float x_translation, float y_translation, float zoom, float fov, float *corners);

stm32ipl_err_t STM32Ipl_Replace(image_t *src, image_t *dst, bool hmirror, bool vflip, bool transpose, image_t *mask);

// Flip, mirror and 90°, 180° and 270° rotations.
stm32ipl_err_t STM32Ipl_Flip(image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Mirror(image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Rotation90(image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Rotation180(image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Rotation270(image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_FlipMirror(image_t *src, image_t *dst);

// Gaussian filter.
stm32ipl_err_t STM32Ipl_Gaussian(image_t *img, uint8_t ksize, bool threshold, bool unsharp);

// Laplacian filter.
stm32ipl_err_t STM32Ipl_Laplacian(image_t *img, uint8_t ksize, bool sharpen);

// Sobel filter.
stm32ipl_err_t STM32Ipl_Sobel(image_t *img, uint8_t ksize, bool sharpen);

// Scharr filter.
stm32ipl_err_t STM32Ipl_Scharr(image_t *img, uint8_t ksize, bool sharpen);

// Kernel filter.
stm32ipl_err_t STM32Ipl_Morph(image_t *img, int ksize, int *krn, float mul, /* CMARCH float*/
int add, bool threshold, int offset, bool invert, image_t *mask);

// Canny edge filter.
stm32ipl_err_t STM32Ipl_EdgeCanny(image_t *grayscale_img, uint8_t min_threshold, uint8_t max_threshold);

// Hough edge circle filter.
stm32ipl_err_t STM32Ipl_FindCircles(image_t *img, rectangle_t *roi, list_t *out, uint32_t x_stride, uint32_t y_stride,
		uint32_t threshold, uint32_t x_margin, uint32_t y_margin, uint32_t r_margin, uint32_t r_min, uint32_t r_max,
		uint32_t r_step);

// Morphological operators.
stm32ipl_err_t STM32Ipl_Dilate(image_t *img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_Erode(image_t *img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_Open(image_t *img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_Close(image_t *img, uint8_t ksize, uint8_t threshold);
stm32ipl_err_t STM32Ipl_TopHat(image_t *img, uint8_t ksize, uint8_t threshold, image_t *mask);
stm32ipl_err_t STM32Ipl_BlackHat(image_t *img, uint8_t ksize, uint8_t threshold, image_t *mask);

//Boolean.
stm32ipl_err_t STM32Ipl_B_and(image_t *imgA, image_t *imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_nand(image_t *imgA, image_t *imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_or(image_t *imgA, image_t *imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_nor(image_t *imgA, image_t *imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_xor(image_t *imgA, image_t *imgB, image_t *mask);
stm32ipl_err_t STM32Ipl_B_xnor(image_t *imgA, image_t *imgB, image_t *mask);

// Histogram
stm32ipl_err_t STM32Ipl_GetHistogram(image_t *img, histogram_t *hist, rectangle_t *roi);

//Statistics
stm32ipl_err_t STM32Ipl_GetStatistics(image_t *img, statistics_t *stats, rectangle_t *roi);

//Gamma Correction
stm32ipl_err_t STM32Ipl_GammaCorr(image_t *img, float gamma_val, float contrast, float brightness);

#endif  // __STM32IPL_H_
