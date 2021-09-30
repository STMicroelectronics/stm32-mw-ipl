/**
 ******************************************************************************
 * @file   stm32ipl.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library functions header file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * Portions of this file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef __STM32IPL_H_
#define __STM32IPL_H_

#include "stm32ipl_conf.h"
#include "imlib.h"


#ifndef M_PI
//#define M_PI    3.14159265f
#define M_PI    3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2  1.57079632f
#endif

#define STM32IPL_UNUSED(x)		((void)(x))
#define STM32IPL_MAX(a, b)      ((a) > (b) ? (a) : (b))
#define STM32IPL_MIN(a, b)      ((a) < (b) ? (a) : (b))
#define STM32IPL_DEG2RAD(x)     (((x) * M_PI) / 180)
#define STM32IPL_RAD2DEG(x)     (((x) * 180) / M_PI)

#define STM32IPL_COLOR_BLUE          0xFF0000FFUL
#define STM32IPL_COLOR_GREEN         0xFF00FF00UL
#define STM32IPL_COLOR_RED           0xFFFF0000UL
#define STM32IPL_COLOR_CYAN          0xFF00FFFFUL
#define STM32IPL_COLOR_MAGENTA       0xFFFF00FFUL
#define STM32IPL_COLOR_YELLOW        0xFFFFFF00UL
#define STM32IPL_COLOR_LIGHTBLUE     0xFF8080FFUL
#define STM32IPL_COLOR_LIGHTGREEN    0xFF80FF80UL
#define STM32IPL_COLOR_LIGHTRED      0xFFFF8080UL
#define STM32IPL_COLOR_LIGHTCYAN     0xFF80FFFFUL
#define STM32IPL_COLOR_LIGHTMAGENTA  0xFFFF80FFUL
#define STM32IPL_COLOR_LIGHTYELLOW   0xFFFFFF80UL
#define STM32IPL_COLOR_DARKBLUE      0xFF000080UL
#define STM32IPL_COLOR_DARKGREEN     0xFF008000UL
#define STM32IPL_COLOR_DARKRED       0xFF800000UL
#define STM32IPL_COLOR_DARKCYAN      0xFF008080UL
#define STM32IPL_COLOR_DARKMAGENTA   0xFF800080UL
#define STM32IPL_COLOR_DARKYELLOW    0xFF808000UL
#define STM32IPL_COLOR_WHITE         0xFFFFFFFFUL
#define STM32IPL_COLOR_LIGHTGRAY     0xFFD3D3D3UL
#define STM32IPL_COLOR_GRAY          0xFF808080UL
#define STM32IPL_COLOR_DARKGRAY      0xFF404040UL
#define STM32IPL_COLOR_BLACK         0xFF000000UL
#define STM32IPL_COLOR_BROWN         0xFFA52A2AUL
#define STM32IPL_COLOR_ORANGE        0xFFFFA500UL


/*
 * @brief STM32IPL color type. It has 0xFFRRGGBB format.
 * STM32IPL_COLOR_XXX colors follow such format.
 */
typedef uint32_t stm32ipl_color_t;

/*
 * @brief STM32IPL error types.
 */
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

/*
 * @brief The image formats supported by this library.
 */
typedef enum _stm32ipl_if_t
{
	stm32ipl_if_binary    = 1,
	stm32ipl_if_grayscale = 2,
	stm32ipl_if_rgb565    = 4,
	stm32ipl_if_rgb888    = 8,
} stm32ipl_if_t;

#define STM32IPL_IF_ALL		(stm32ipl_if_binary | stm32ipl_if_grayscale | stm32ipl_if_rgb565 | stm32ipl_if_rgb888)
#define STM32IPL_IF_RGB		(stm32ipl_if_rgb565 | stm32ipl_if_rgb888)
#define STM32IPL_IF_NOT_RGB	(stm32ipl_if_binary | stm32ipl_if_grayscale)
#define STM32IPL_IF_NOT_RGB888 (stm32ipl_if_binary | stm32ipl_if_grayscale | stm32ipl_if_rgb565)

#define STM32IPL_CHECK_FORMAT(img, formats) \
if (!STM32Ipl_ImageFormatSupported((img), (formats))) \
	return stm32ipl_err_UnsupportedFormat; \

#define STM32IPL_CHECK_ROI(img, roi) \
{ \
	rectangle_t fullRoi; \
	rectangle_init(&fullRoi, 0, 0, img->w, img->h); \
	if (!STM32Ipl_RectContain(&fullRoi, roi)) \
		return stm32ipl_err_WrongROI; \
}

#define STM32IPL_CHECK_VALID_IMAGE(img) \
{ \
	if (!img || !img->data) \
		return stm32ipl_err_InvalidParameter; \
}

#define STM32IPL_CHECK_SAME_RESOLUTION(src, dst) \
{ \
	if ((src->w != dst->w) || (src->h != dst->h)) \
		return stm32ipl_err_InvalidParameter; \
}

#define STM32IPL_CHECK_SAME_FORMAT(src, dst) \
{ \
	if (src->bpp != dst->bpp) \
		return stm32ipl_err_InvalidParameter; \
}

#define STM32IPL_CHECK_EQUAL(src, dst) \
{ \
	if ((src->w != dst->w) || (src->h != dst->h) || (src->bpp != dst->bpp)) \
		return stm32ipl_err_InvalidParameter; \
}

/*
 * @brief Represents a rotated rectangles on a plane.
 * Each rectangle is specified by the center point (mass center), length of each side (width and height)
 * and the rotation angle expressed in degrees.
 */
typedef struct _rotatedRect_t {
	point_t center; 	/**< Coordinates of the center of the rectangle*/
	int16_t w; 			/**< Width of the rectangle. */
    int16_t h;			/**< Height of the rectangle. */
    int16_t rotation;	/**< Rotation angle (degrees). */
} rotatedRect_t;

/*
 * @brief Represents an ellipse on a plane.
 * Each ellipse is specified by the center point (mass center), length of each semi-axis (semi-major and semi-minor axis)
 * and the rotation angle expressed in degrees.
 */
typedef struct _ellipse_t {
	point_t center; 	/**< Coordinates of the center of the ellipse.*/
	int16_t radius_x; 	/**< Width of radius. */
    int16_t radius_y;	/**< Height of radius. */
    int16_t rotation;	/**< Rotation angle (degrees). */
} ellipse_t;


		/*
		 * Library initialization.
		 */
		void STM32Ipl_InitLib(void *memAddr, uint32_t memSize);
		void STM32Ipl_DeInitLib(void);

		/*
		 * Image initialization and support.
		 */
		void STM32Ipl_Init(image_t *img, uint32_t width, uint32_t height, image_bpp_t format, void *data);
		stm32ipl_err_t STM32Ipl_AllocData(image_t *img, uint32_t width, uint32_t height, image_bpp_t format);
		stm32ipl_err_t STM32Ipl_AllocDataRef(const image_t *src, image_t *dst);
		void STM32Ipl_ReleaseData(image_t *img);
		uint32_t STM32Ipl_DataSize(uint32_t width, uint32_t height, image_bpp_t format);
		uint32_t STM32Ipl_ImageDataSize(const image_t *img);
		bool STM32Ipl_ImageFormatSupported(const image_t *img, uint32_t formats);
		stm32ipl_err_t STM32Ipl_Copy(const image_t *src, image_t *dst);
		stm32ipl_err_t STM32Ipl_CopyData(const image_t *src, image_t *dst);
		stm32ipl_err_t STM32Ipl_Clone(const image_t *src, image_t *dst);
		stm32ipl_err_t STM32Ipl_Zero(image_t *img, bool invert, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Fill(image_t *img, uint32_t value, const rectangle_t *roi);

/*
 * Image conversion.
 */
stm32ipl_err_t STM32Ipl_Convert(const image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_ConvertWithOverlap(const image_t *src, image_t *dst);

/*
 * Image cropping and scaling.
 */
stm32ipl_err_t STM32Ipl_Crop(const image_t *src, image_t *dst, uint32_t x, uint32_t y);
stm32ipl_err_t STM32Ipl_Resize(const image_t *src, image_t *dst, const rectangle_t *roi);
stm32ipl_err_t STM32Ipl_Downscale(const image_t *src, image_t *dst, bool reversed);


/*
 * Image equalization.
 */
	stm32ipl_err_t STM32Ipl_HistEq(image_t *img, const image_t *mask);
	stm32ipl_err_t STM32Ipl_ClaheHistEq(image_t *img, float clipLimit, const image_t *mask);


/*
 * Image ?????????????.
 */
	stm32ipl_err_t STM32Ipl_Binary(const image_t *src, image_t *dst, list_t *thresholds, bool invert, bool zero, const image_t *mask);
	stm32ipl_err_t STM32Ipl_Invert(image_t *img);

/*
 * Image rotation and transformation.
 */
stm32ipl_err_t STM32Ipl_Rotation(image_t *img, float rotationX, float rotationY, float rotationZ, float translationX, float translationY, float zoom, float fov, const float *corners);
stm32ipl_err_t STM32Ipl_Replace(const image_t *src, image_t *dst, bool mirror, bool flip, bool transpose, const image_t *mask);
stm32ipl_err_t STM32Ipl_Flip(const image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Mirror(const image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_FlipMirror(const image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Rotation90(const image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Rotation180(const image_t *src, image_t *dst);
stm32ipl_err_t STM32Ipl_Rotation270(const image_t *src, image_t *dst);


/*
 * Rectangle.
 */
		stm32ipl_err_t STM32Ipl_RectInit(rectangle_t *r, int16_t x, int16_t y, int16_t width, int16_t height);
		rectangle_t* STM32Ipl_RectAlloc(int16_t x, int16_t y, int16_t width, int16_t height);
		void STM32Ipl_RectFree(rectangle_t **r);
		stm32ipl_err_t STM32Ipl_RectCopy(const rectangle_t *src, rectangle_t *dst);
		bool STM32Ipl_RectEqual(const rectangle_t *r0, const rectangle_t *r1);
		bool STM32Ipl_RectangleEqualFast(const rectangle_t *r0, const rectangle_t *r1);
		bool STM32Ipl_RectContain(const rectangle_t *r0, const rectangle_t *r1);
		bool STM32Ipl_RectOverlap(const rectangle_t *r0, const rectangle_t *r1);
		stm32ipl_err_t STM32Ipl_RectIntersected(const rectangle_t *src, rectangle_t *dst);
		stm32ipl_err_t STM32Ipl_RectUnited(const rectangle_t *src, rectangle_t *dst);
		stm32ipl_err_t STM32Ipl_RectExpand(rectangle_t *r, uint16_t x, uint16_t y);
		bool STM32Ipl_RectSubImage(const image_t *img, const rectangle_t *src, rectangle_t *dst);
		stm32ipl_err_t STM32Ipl_RectToPoints(const rectangle_t *r, point_t *points);
		stm32ipl_err_t STM32Ipl_RectMerge(array_t **rects);


/*
 * Object detection.
 */
#ifdef STM32IPL_ENABLE_OBJECT_DETECTION
stm32ipl_err_t STM32Ipl_LoadCascadeFromMemory(cascade_t *cascade, const uint8_t *memory);
#ifdef STM32IPL_USE_FRONTAL_FACE_CASCADE
stm32ipl_err_t STM32Ipl_LoadFaceCascade(cascade_t *cascade);
#endif /* STM32IPL_USE_FRONTAL_FACE_CASCADE */
#ifdef STM32IPL_USE_EYE_CASCADE
stm32ipl_err_t STM32Ipl_LoadEyeCascade(cascade_t *cascade);
#endif /* STM32IPL_USE_EYE_CASCADE */
stm32ipl_err_t STM32Ipl_DetectObject(const image_t *img, cascade_t *cascade, const rectangle_t *roi, float scaleFactor, float threshold, array_t **objects);
#endif /* STM32IPL_ENABLE_OBJECT_DETECTION */


/*******************************************************
 *******************************************************
 * TODO: from this point on, the code must be reviewed.
 *******************************************************
 *******************************************************/
/////////// ULTIMO

/*
 * Line.
 */
		bool STM32Ipl_ClipLine(line_t *l, int16_t x, int16_t y, uint16_t w, uint16_t h);


/*
 * Boolean.
 */
		stm32ipl_err_t STM32Ipl_And(image_t *imgA, const image_t *imgB, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Nand(image_t *imgA, const image_t *imgB, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Or(image_t *imgA, const image_t *imgB, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Nor(image_t *imgA, const image_t *imgB, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Xor(image_t *imgA, const image_t *imgB, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Xnor(image_t *imgA, const image_t *imgB, const image_t *mask);


/*
 * Math.
 */
		stm32ipl_err_t STM32Ipl_Add(image_t *img, const image_t *other, uint32_t scalar, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Sub(image_t *img, const image_t *other, uint32_t scalar, bool reverse, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Mul(image_t *img, const image_t *other, uint32_t scalar, bool reverse, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Div(image_t *img, const image_t *other, uint32_t scalar, bool reverse, bool mod, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Min(image_t *img, const image_t *other, uint32_t scalar, const image_t *mask);
		stm32ipl_err_t STM32Ipl_Max(image_t *img, const image_t *other, uint32_t scalar, const image_t *mask);
		stm32ipl_err_t STM32Ipl_GammaCorr(image_t *img, float gamma_val, float contrast, float brightness);


/*
 * Morphological.
 */
		stm32ipl_err_t STM32Ipl_Dilate(image_t *img, uint8_t ksize, uint8_t threshold);
		stm32ipl_err_t STM32Ipl_Erode(image_t *img, uint8_t ksize, uint8_t threshold);
		stm32ipl_err_t STM32Ipl_Open(image_t *img, uint8_t ksize, uint8_t threshold);
		stm32ipl_err_t STM32Ipl_Close(image_t *img, uint8_t ksize, uint8_t threshold);
		stm32ipl_err_t STM32Ipl_TopHat(image_t *img, uint8_t ksize, uint8_t threshold, const image_t *mask);
		stm32ipl_err_t STM32Ipl_BlackHat(image_t *img, uint8_t ksize, uint8_t threshold, const image_t *mask);


/*
 * Filtering.
 */
		stm32ipl_err_t STM32Ipl_MeanFilter(image_t *img, const int32_t ksize, bool threshold, int32_t offset, bool invert, const image_t *mask);
		stm32ipl_err_t STM32Ipl_MedianFilter(image_t *img, int ksize, float percentile, bool threshold, int offset, bool invert, const image_t *mask);
		stm32ipl_err_t STM32Ipl_ModeFilter(image_t *img, const int ksize, bool threshold, int offset, bool invert, const image_t *mask);
		stm32ipl_err_t STM32Ipl_MidpointFilter(image_t *img, const int ksize, float bias, bool threshold, int offset, bool invert, const image_t *mask);
		stm32ipl_err_t STM32Ipl_BilateralFilter(image_t *img, const int ksize, float color_sigma, float space_sigma, bool threshold, int offset, bool invert, const image_t *mask);
stm32ipl_err_t STM32Ipl_Gaussian(image_t *img, uint8_t ksize, bool threshold, bool unsharp);
stm32ipl_err_t STM32Ipl_Laplacian(image_t *img, uint8_t ksize, bool sharpen);
stm32ipl_err_t STM32Ipl_Sobel(image_t *img, uint8_t ksize, bool sharpen);
stm32ipl_err_t STM32Ipl_Scharr(image_t *img, uint8_t ksize, bool sharpen);
stm32ipl_err_t STM32Ipl_Morph(image_t *img, int ksize, int *krn, float mul, int add, bool threshold, int offset, bool invert, const image_t *mask);


/*
 * Edge and shape detection.
 */
		stm32ipl_err_t STM32Ipl_EdgeCanny(image_t *grayscale_img, uint8_t min_threshold, uint8_t max_threshold);
		stm32ipl_err_t STM32Ipl_FindCircles(const image_t *img, const rectangle_t *roi, list_t *out, uint32_t x_stride, uint32_t y_stride, uint32_t threshold, uint32_t x_margin, uint32_t y_margin, uint32_t r_margin, uint32_t r_min, uint32_t r_max, uint32_t r_step);

		stm32ipl_err_t STM32Ipl_FindLines(list_t *out, const image_t *ptr, const rectangle_t *roi, uint8_t x_stride, uint8_t y_stride, uint32_t threshold, uint8_t theta_margin, uint8_t rho_margin);
		stm32ipl_err_t STM32Ipl_LineLength(const find_lines_list_lnk_data_t *lnk_data, uint32_t *length);

/*
 * Color tracking.
 */
		stm32ipl_err_t STM32Ipl_FindBlobs(const image_t *img, list_t *out, const list_t *thresholds, const rectangle_t *roi, uint8_t x_stride, uint8_t y_stride, uint16_t area_threshold, uint16_t pixels_threshold, bool merge, uint8_t margin, bool invert);



stm32ipl_err_t STM32Ipl_EnclosingCircle(const point_t *point, uint16_t n_points, point_t *c, uint16_t *dmax);



/*
 * Warping.
 */
stm32ipl_err_t STM32Ipl_GetAffineTransform(const point_t *src, const point_t *dst, array_t *affine);
stm32ipl_err_t STM32Ipl_WarpAffine(image_t *img, const array_t *affine);
stm32ipl_err_t STM32Ipl_WarpAffinePoints(array_t *points, const array_t *affine);




/*
 * Pixel.
 */
		stm32ipl_err_t STM32Ipl_GetPixel(const image_t *img, uint16_t x, uint16_t y, int *p);
		bool STM32Ipl_GetMaskPixel(const image_t *ptr, uint16_t x, uint16_t y);



/*
 * Drawing.
 */
		stm32ipl_err_t STM32Ipl_DrawPixel(image_t *img, uint16_t x, uint16_t y, uint32_t color);
		stm32ipl_err_t STM32Ipl_DrawCross(image_t * img, uint16_t x, uint16_t y, uint8_t size, uint32_t color, uint8_t thickness);
		stm32ipl_err_t STM32Ipl_DrawLine(image_t * img, point_t * p0, point_t * p1, uint32_t color , uint8_t thickness);
		stm32ipl_err_t STM32Ipl_DrawPolygon(image_t * img, point_t *point, uint32_t n_points, uint32_t color , uint8_t thickness);
		stm32ipl_err_t STM32Ipl_DrawRectangle(image_t * img, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color , uint8_t thickness, bool fill);
		stm32ipl_err_t STM32Ipl_DrawCircle(image_t *img, uint16_t cx, uint16_t cy, uint16_t radius, uint32_t color, uint8_t thickness, bool fill);
		stm32ipl_err_t STM32Ipl_DrawEllipse(image_t *img, uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, uint16_t rotation, uint32_t color, uint8_t thickness, bool fill);


/*
 * Statistics.
 */
		stm32ipl_err_t STM32Ipl_GetSimilarity(const image_t *img, const image_t *other, int scalar, float *avg, float *std, float *min, float *max);
		stm32ipl_err_t STM32Ipl_GetPercentile(const histogram_t *ptr, image_bpp_t bpp, percentile_t *out, float percentile);
		stm32ipl_err_t STM32Ipl_GetThreshold(const histogram_t *ptr, image_bpp_t bpp, threshold_t *out);
		stm32ipl_err_t STM32Ipl_GetHistogram(const image_t *img, histogram_t *hist, const rectangle_t *roi);
		stm32ipl_err_t STM32Ipl_GetStatistics(const image_t *img, statistics_t *stats, const rectangle_t *roi);
		stm32ipl_err_t STM32Ipl_GetRegressionImage(find_lines_list_lnk_data_t *out, const image_t *img, const rectangle_t *roi, uint8_t x_stride, uint8_t y_stride, const list_t *thresholds, bool invert, unsigned int area_threshold,	unsigned int pixels_threshold, bool robust);
		stm32ipl_err_t STM32Ipl_GetRegressionPoints(find_lines_list_lnk_data_t *out, const point_t *points, uint16_t n_points,bool robust);
		stm32ipl_err_t STM32Ipl_ImageMean(const image_t *src, int *r_mean, int *g_mean, int *b_mean);
		stm32ipl_err_t STM32Ipl_ImageStd(const image_t *src, int16_t *grayscale_std);
		stm32ipl_err_t STM32Ipl_MidpointPool(const image_t *src, image_t *dst, const int bias);
		stm32ipl_err_t STM32Ipl_MidpointPooled(image_t *img, int x_div, int y_div, const int bias);
		stm32ipl_err_t STM32Ipl_MeanPool(const image_t *src, image_t *dst);
		stm32ipl_err_t STM32Ipl_MeanPooled(image_t *img, int x_div, int y_div);


/*
 * Template matching.
 */
		stm32ipl_err_t STM32Ipl_FindTemplate(const image_t *img, const image_t *template, float thresh, rectangle_t *roi, int step, int search, rectangle_t *findedTemplate, float *corrFindedTemplate);
		stm32ipl_err_t STM32Ipl_FindDisplacement(const image_t *img, const rectangle_t *roi, const image_t *template, const rectangle_t *roi_template, bool logpolar, bool fix_rotation_scale, float *x_translation, float *y_translation, float *rotation, float *scale, float *response);

//Array
stm32ipl_err_t STM32Ipl_CountNonZero(const image_t *img, uint32_t *result);
stm32ipl_err_t STM32Ipl_FindNonZero(const image_t *img, list_t *result, rectangle_t *roi);
stm32ipl_err_t STM32Ipl_MinMaxLoc(const image_t *img, list_t *resultMin, list_t *resultMax);
stm32ipl_err_t STM32Ipl_FindBlobs(const image_t *img, list_t *out, const list_t *thresholds, const rectangle_t *roi, uint8_t x_stride, uint8_t y_stride, uint16_t area_threshold, uint16_t pixels_threshold, bool merge, uint8_t margin, bool invert);


stm32ipl_err_t STM32Ipl_Svd(const float *pointx, const float *pointy, uint16_t n_points, float *u, float *s, float *v);


//Common
stm32ipl_err_t STM32Ipl_BoxPoints(const rotatedRect_t *r, point_t *points);
stm32ipl_err_t STM32Ipl_EnclosingCircle(const point_t *point, uint16_t n_points, point_t *c, uint16_t *dmax);
stm32ipl_err_t STM32Ipl_EnclosingEllipse(const point_t *point, uint16_t n_points, point_t *c, float *max, float *min,
		float *rotation);

stm32ipl_err_t STM32Ipl_FitEllipse(const uint16_t *x, const uint16_t *y, uint16_t n_points, float *fit, uint8_t M,
		ellipse_t *ellipse);
stm32ipl_err_t STM32Ipl_ArcLength(const point_t *points, uint16_t count, bool is_closed, float *perimeter);


/*
 * Point.
 */
		stm32ipl_err_t STM32Ipl_PointInit(point_t *ptr, uint16_t x, uint16_t y);
		point_t *STM32Ipl_PointAlloc(int16_t x, int16_t y);
		void STM32Ipl_PointFree(point_t * ptr);
		stm32ipl_err_t STM32Ipl_PointCopy(const point_t *src, point_t *dst);
		bool STM32Ipl_PointEqualFast(const point_t *ptr0, const point_t *ptr1);
		bool STM32Ipl_PointEqual(const point_t *ptr0, const point_t *ptr1);
		stm32ipl_err_t STM32Ipl_PointDistance(const point_t *ptr0, const point_t *ptr1, float *result);
		stm32ipl_err_t STM32Ipl_PointQuadrance(const point_t *ptr0, const point_t *ptr1, int *quadrance);
		stm32ipl_err_t STM32Ipl_PointRotate(int16_t x, int16_t y, uint16_t degree, int16_t center_x, int16_t center_y, int16_t *new_x, int16_t *new_y);
		stm32ipl_err_t STM32Ipl_PointMinAreaRectangle(const point_t *corners, point_t *new_corners, uint16_t corners_len);


// Iris detector
stm32ipl_err_t STM32Ipl_FindIris(const image_t *src, point_t *iris, const rectangle_t *roi);

// HoG
stm32ipl_err_t STM32Ipl_FindHog(image_t *src, const rectangle_t *roi, uint8_t cell_size);



/*
 * Correction.
 */
		stm32ipl_err_t STM32Ipl_Logpolar(image_t *img, bool reverse);
		stm32ipl_err_t STM32Ipl_Linpolar(image_t *img, bool reverse);


stm32ipl_err_t STM32Ipl_ImageMaskRectangle(image_t *img, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
stm32ipl_err_t STM32Ipl_ImageMaskCircle(image_t *img, uint16_t cx, uint16_t cy, uint16_t radius);
stm32ipl_err_t STM32Ipl_ImageMaskEllipse(image_t *img, uint16_t cx, uint16_t cy, uint16_t xradius, uint16_t yradius, uint16_t rotation);

/*
 * Lens correction.
 */
		stm32ipl_err_t STM32Ipl_LensCorr(image_t *img, float strength, float zoom, float x_corr, float y_corr);

#endif  // __STM32IPL_H_
