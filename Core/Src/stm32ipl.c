/**
 ******************************************************************************
 * @file   stm32ipl.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library functions
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

#include "stm32ipl.h"

///** TODO: keep this?
// * @brief Represents a rectangle with integer values.
// * Origin of the coordinates (top-left) is (0, 0).
// * (x, y) is the position of the rectangle.
// * (w, h) is the size of the rectangle.
//*/
//typedef struct _stm32ipl_rect_t
//{
//	int32_t x;	/* X-coordinate of the top-left corner of the rectangle. */
//	int32_t y;	/* Y-coordinate of the top-left corner of the rectangle. */
//	int32_t w;	/* Width of the rectangle. */
//	int32_t h;	/* Height of the rectangle. */
//} stm32ipl_rect_t;

/*
 * @brief Initializes a rectangle.
 * param rect The rectangle to be set.
 * @param x			X-coordinate of the top-left corner of the rectangle.
 * @param y			Y-coordinate of the top-left corner of the rectangle.
 * @param width		Width of the rectangle.
 * @param height	Height of the rectangle.
 * @return			void.
 */
void STM32Ipl_RectInit(rectangle_t *rect, int32_t x, int32_t y, int32_t width, int32_t height)
{
	if (rect) {
		rect->x = x;
		rect->y = y;
		rect->w = width;
		rect->h = height;
	}
}

/*
 * @brief Copies the source rectangle data to the destination one.
 * @param src	Source rectangle.
 * @param dst	Destination rectangle.
 * @return		void.
 */
void STM32Ipl_RectCopy(const rectangle_t *src, rectangle_t *dst)
{
	if (src && dst)
		memcpy(dst, src, sizeof(rectangle_t));
}

/*
 * @brief Determines whether the two given rectangles overlap.
 * @param rect0 First rectangle.
 * @param rect1 Second rectangle.
 * @return		True if the two rectangles overlap, false otherwise.
 */
bool STM32Ipl_RectOverlap(const rectangle_t *rect0, const rectangle_t *rect1)
{
	if (rect0 && rect1)
		return ((rect0->x < (rect1->x + rect1->w)) && (rect1->x < (rect0->x + rect0->w))
				&& (rect0->y < (rect1->y + rect1->h)) && (rect1->y < (rect0->y + rect0->h)));

	return false;
}


///*!
// \brief Determines whether the rectangle is empty.
//   A rectangle is defined as empty if its width or height
//   is less than or equal to zero.
// \param rect The rectangle to be considered.
// \return True if the rectangle is empty, false otherwise.
//*/
//bool HmiRectIsEmpty(const rectangle_t* rect)
//{
//	if (rect)
//		return (rect->w <= 0) || (rect->h <= 0);
//
//	return false;
//}

/*!
 \brief Calculates the intersection of two rectangles, and stores the result in another rectangle.
 \param rectA The first rectangle.
 \param rectB The second rectangle.
 \param rectC The intersection of the first and second rectangle.
 \return True if the intersection is not empty, false otherwise.
*/
bool HmiRectIntersect(const rectangle_t* rectA, const rectangle_t* rectB)
{
	if (rectA && rectB) {
		int32_t right = STM32IPL_MIN(rectA->x + rectA->w, rectB->x + rectB->w);
		int32_t bottom = STM32IPL_MIN(rectA->y + rectA->h, rectB->y + rectB->y);
		int32_t left = STM32IPL_MAX(rectA->x, rectB->x);
		int32_t top = STM32IPL_MAX(rectA->y, rectB->y);
		return ((right - left) <= 0) || ((bottom - top) <= 0);
	}

	return false;
}


/*
 * @brief Determines whether the second rectangle is inside the first one.
 * @param rect0 First rectangle.
 * @param rect1 Second rectangle.
 * @return		True if the second rectangle is contained in the first one, false otherwise.
 */
bool STM32Ipl_RectContain(const rectangle_t *rect0, const rectangle_t *rect1)
{
	if (rect0 && rect1)
		return ((rect0->x <= rect1->x) && ((rect1->x + rect1->w) <= (rect0->x + rect0->w)) && (rect0->y <= rect1->y)
				&& ((rect1->y + rect1->h) <= (rect0->y + rect0->h)));

	return false;
}

/*
 * @brief Gets the union between the two given rectangles, and saves it into rect1.
 * @param rect0 First rectangle.
 * @param rect1 Second rectangle.
 * @return		void.
 */
void STM32Ipl_RectUnion(const rectangle_t *rect0, rectangle_t *rect1)
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;

	if (!rect0 || !rect1)
		return;

	left = STM32IPL_MIN(rect0->x, rect1->x);
	top = STM32IPL_MIN(rect0->y, rect1->y);
	right = STM32IPL_MAX(rect0->x + rect0->w, rect1->x + rect1->w);
	bottom = STM32IPL_MAX(rect0->y + rect0->h, rect1->y + rect1->h);

	rect1->x = left;
	rect1->y = top;
	rect1->w = right - left;
	rect1->h = bottom - top;
}

/*
 * @brief Gets the intersection between the two given rectangles, and saves it into rect1.
 * @param rect0 First rectangle.
 * @param rect1 Second rectangle.
 * @return		void.
 */
void STM32Ipl_RectIntersect(const rectangle_t *rect0, rectangle_t *rect1)
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;

	if (!rect0 || !rect1)
		return;

	left = STM32IPL_MAX(rect0->x, rect1->x);
	top = STM32IPL_MAX(rect0->y, rect1->y);
	right = STM32IPL_MIN(rect0->x + rect0->w, rect1->x + rect1->w);
	bottom = STM32IPL_MIN(rect0->y + rect0->h, rect1->y + rect1->h);

	rect1->x = left;
	rect1->y = top;
	rect1->w = right - left;
	rect1->h = bottom - top;
}

/*
 * @brief Initializes the memory manager used by this library.
 * @param memAddress	Address of the memory buffer the library will use for its internal purposes.
 * @param memSize		Size of the memory in bytes.
 * @return				void.
 */
void STM32Ipl_InitLib(void *memAddress, uint32_t memSize)
{
	umm_init(/*(void*)*/memAddress, memSize);
	fb_init();
}

/*
 * @brief De-initializes the memory manager of this library.
 * @return	void.
 */
void STM32Ipl_DeInitLib(void)
{
	umm_uninit();
}

/*
 * @brief Initializes the image with the given arguments.
 * @param img		Image.
 * @param width		Image width.
 * @param height	Image height.
 * @param format    Image format.
 * @param data		Pixel data.
 * @return			void.
 */
void STM32Ipl_Init(image_t *img, uint32_t width, uint32_t height, image_bpp_t format, void *data)
{
	if (img) {
		img->w = width;
		img->h = height;
		img->bpp = format;
		img->data = data;
	}
}

///*
// * @brief Resets the whole image structure.
// * @param img	Image.
// * @return		void.
// */
//inline void STM32Ipl_Reset(image_t *img)
//{
//	if (img)
//		memset(img, 0, sizeof(image_t));
//}

/*
 * @brief Allocates a data memory buffer to contain the image pixels and consequently
 * initializes the given image structure.
 * The size of such buffer depends on width, height and format.
 * Assuming the input image data pointer is null to avoid memory leakage.
 * @param img		Image
 * @param width		Image width
 * @param height	Image height
 * @param bpp		Image format
 * @return			stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_AllocateData(image_t *img, uint32_t width, uint32_t height, image_bpp_t format)
{
	uint8_t *data;

	if (!img)
		return stm32ipl_err_InvalidParameter;

	data = xalloc(STM32Ipl_DataSize(width, height, format));
	if (!data) {
		STM32Ipl_Init(img, 0, 0, 0, 0);
		return stm32ipl_err_OutOfMemory;
	}

	img->w = width;
	img->h = height;
	img->bpp = format;
	img->data = data;

	return stm32ipl_err_Ok;
}

/*
 * @brief Releases the data memory buffer of the image and resets the image structure.
 * @param img	Image.
 * @return		void.
 */
void STM32Ipl_ReleaseData(image_t *img)
{
	if (img) {
		xfree(img->data);
		STM32Ipl_Init(img, 0, 0, 0, 0);
	}
}

/*
 * @brief Gets the size of the memory needed to store an image with the given properties.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param width		Image width.
 * @param height	Image height.
 * @param format	Image format.
 * @return			size of the image data buffer (bytes), 0 in case of wrong input.
 */
uint32_t STM32Ipl_DataSize(uint32_t width, uint32_t height, image_bpp_t format)
{
	switch ((uint32_t)format) {
		case IMAGE_BPP_BINARY:
			return ((width + UINT32_T_MASK) >> UINT32_T_SHIFT) * height * sizeof(uint32_t);

		case IMAGE_BPP_GRAYSCALE:
			return width * height * sizeof(uint8_t);

		case IMAGE_BPP_RGB565:
			return width * height * sizeof(uint16_t);

		case IMAGE_BPP_BAYER:
			return width * height * sizeof(uint8_t);

		case IMAGE_BPP_RGB888:
			return width * height * 3;
	}

	return 0;
}

/*
 * @brief Gets the size the data buffer of the given image (in bytes).
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	Image.
 * @return		size of the image data buffer (bytes), 0 in case of wrong input.
 */
uint32_t STM32Ipl_ImageDataSize(const image_t *img)
{
	return img ? STM32Ipl_DataSize(img->w, img->h, img->bpp) : 0;
}

/*
 * @brief Copies the source image into the destination one. Only the image structure is copied,
 * so beware the source image data will be shared with the destination image, as no new memory
 * buffer is allocated.
 * @param src	Source image.
 * @param dst   Destination image (assuming its data pointer is null to avoid memory leakage).
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Copy(const image_t *src, image_t *dst)
{
	if (!src || !src->data || !dst)
		return stm32ipl_err_InvalidParameter;

	memcpy(dst, src, sizeof(image_t));

	return stm32ipl_err_Ok;
}

/*
 * @brief Copies the source image data buffer into the destination one.
 * Only the pixel data is actually copied. Source and destination images must have same resolution and format.
 * The destination image data pointer must refer to a valid memory buffer as no new memory is allocated.
 * @param src	Source image.
 * @param dst   Destination image (assuming its data pointer refers to a valid buffer).
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_CopyData(const image_t *src, image_t *dst)
{
	if (!src || !src->data || !dst || !src->data)
		return stm32ipl_err_InvalidParameter;

	if ((src->w != dst->w) || (src->h != dst->h) || (src->bpp != dst->bpp))
		return stm32ipl_err_InvalidParameter;

	memcpy(dst->data, src->data, STM32Ipl_ImageDataSize(dst));

	return stm32ipl_err_Ok;
}

/*
 * @brief Clones the source image into the destination one. If the destination image data pointer
 * is null, a new memory buffer is allocated, filled with the source pixel data and assigned to
 * the destination image. If the destination image data pointer points to a user allocated buffer,
 * such buffer must have the right size to contain the source image. In case of success, the two
 * images will have same resolution and format.
 * @param src	Source image.
 * @param dst   Destination image.
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Clone(const image_t *src, image_t *dst)
{
	uint8_t *data;
	size_t size;

	if (!src || !src->data || !dst)
		return stm32ipl_err_InvalidParameter;

	if (dst->data) {
		if ((src->w != dst->w) || (src->h != dst->h) || (src->bpp != dst->bpp))
			return stm32ipl_err_InvalidParameter;
	} else {
		size = STM32Ipl_ImageDataSize(src);

		data = xalloc(size);
		if (!data) {
			STM32Ipl_Init(dst, 0, 0, 0, 0);
			return stm32ipl_err_OutOfMemory;
		}

		dst->w = src->w;
		dst->h = src->h;
		dst->bpp = src->bpp;
		dst->data = data;
	}

	memcpy(dst->data, src->data, size);

	return stm32ipl_err_Ok;
}

/*
 * @brief Sets the image data buffer to zero.
 * @param img	Image.
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Zero(image_t *img)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	memset(img->data, 0, STM32Ipl_ImageDataSize(img));

	return stm32ipl_err_Ok;
}

/*
 * @brief Fills the image with the given color.
 * If roi is defined, only the corresponding region is filled.
 * @param img	Image.
 * @param color	Color.
 * @param roi	Region of interest of the source image: it must be contained in the source image and have
 * positive dimensions, otherwise an error is returned.
  * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Fill(image_t *img, int color, const rectangle_t *roi)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	// check roi dentro img,
	if (roi) {
		for (uint32_t y = roi->y, yy = roi->y + roi->h; y < yy; y++) {
		    for (uint32_t x = roi->x, xx = roi->x + roi->w; x < xx; x++) {
		        imlib_set_pixel(img, x, y, color);
		    }
		}
	} else {
		for (uint32_t y = 0, yy = img->h; y < yy; y++) {
		    for (uint32_t x = 0, xx = img->w; x < xx; x++) {
		        imlib_set_pixel(img, x, y, color);
		    }
		}
	}

	return stm32ipl_err_Ok;
}


/*
 * brief Copies the source image pixels to the destination image buffer.
 * The two buffers must have the same size.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param size	 Size (in bytes) of the image buffer.
 * param reverse Forces the reversed processing (from the last to the first pixel).
 * return		 void.
 */
static void STM32Ipl_SimpleCopy(const uint8_t *src, uint8_t *dst, uint32_t size, bool reverse)
{
	if (reverse) {
		src += size;
		dst += size;
		for (uint32_t i = 0; i < size; i++)
			*dst-- = *src--;
	} else {
		for (uint32_t i = 0; i < size; i++)
			*dst++ = *src++;
	}
}

/*
 * brief Converts the source image pixels from Binary to Grayscale and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst   	 Destination image data.
 * param width	 Width of the two images.
 * param height	 Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_BinaryToY8(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t *srcData = (uint32_t*)src;
	uint32_t srcRowLen = (width + UINT32_T_MASK) >> UINT32_T_SHIFT;

	if (reverse) {
		srcData += srcRowLen * (height - 1);
		dst += (width * height) - 1;
		for (uint32_t y = 0; y < height; y++) {
			for (int32_t x = width - 1; x >= 0; x--)
				*dst-- = COLOR_BINARY_TO_GRAYSCALE(IMAGE_GET_BINARY_PIXEL_FAST(srcData, x));

			srcData -= srcRowLen;
		}

	} else {
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++)
				*dst++ = COLOR_BINARY_TO_GRAYSCALE(IMAGE_GET_BINARY_PIXEL_FAST(srcData, x));

			srcData += srcRowLen;
		}
	}
}

/*
 * brief Converts the source image pixels from Binary to RGB565 and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_BinaryToRGB565(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t *srcData = (uint32_t*)src;
	uint32_t srcRowLen = (width + UINT32_T_MASK) >> UINT32_T_SHIFT;
	uint16_t *dstData = (uint16_t*)dst;

	if (reverse) {
		srcData += srcRowLen * (height - 1);
		dstData += (width * height) - 1;
		for (uint32_t y = 0; y < height; y++) {
			for (int32_t x = width - 1; x >= 0; x--)
				*dstData-- = COLOR_BINARY_TO_RGB565(IMAGE_GET_BINARY_PIXEL_FAST(srcData, x));

			srcData -= srcRowLen;
		}
	} else {
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++)
				*dstData++ = COLOR_BINARY_TO_RGB565(IMAGE_GET_BINARY_PIXEL_FAST(srcData, x));

			srcData += srcRowLen;
		}
	}
}

/*
 * brief Converts the source image pixels from Binary to RGB888 and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_BinaryToRGB888(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t *srcData = (uint32_t*)src;
	uint32_t srcRowLen = (width + UINT32_T_MASK) >> UINT32_T_SHIFT;

	if (reverse) {
		srcData += srcRowLen * (height - 1);
		dst += (width * height * 3) - 1;
		for (uint32_t y = 0; y < height; y++) {
			for (int32_t x = width - 1; x >= 0; x--) {
				uint8_t v = 0xFF * IMAGE_GET_BINARY_PIXEL_FAST(srcData, x);
				*dst-- = v;
				*dst-- = v;
				*dst-- = v;
			}
			srcData -= srcRowLen;
		}

	} else {
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {
				uint8_t v = 0xFF * IMAGE_GET_BINARY_PIXEL_FAST(srcData, x);
				*dst++ = v;
				*dst++ = v;
				*dst++ = v;
			}

			srcData += srcRowLen;
		}
	}
}

/*
 * brief Converts the source image pixels from Grayscale to RGB565 and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_Y8ToBinary(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t *dstData = (uint32_t*)dst;
	uint32_t dstRowLen = (width + UINT32_T_MASK) >> UINT32_T_SHIFT;

	if (reverse) {
		src += (width * height) - 1;
		dstData += dstRowLen * (height - 1);

		for (uint32_t y = 0; y < height; y++) {
			for (int32_t x = width - 1; x >= 0; x--)
				IMAGE_PUT_BINARY_PIXEL_FAST(dstData, x, COLOR_GRAYSCALE_TO_BINARY(*src--));

			dstData -= dstRowLen;
		}

	} else {
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++)
				IMAGE_PUT_BINARY_PIXEL_FAST(dstData, x, COLOR_GRAYSCALE_TO_BINARY(*src++));

			dstData += dstRowLen;
		}
	}
}

/*
 * brief Converts the source image pixels from Grayscale to RGB565 and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_Y8ToRGB565(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint16_t *dstData = (uint16_t*)dst;
	uint32_t size = width * height;

	if (reverse) {
		src += (width * height) - 1;
		dstData += (width * height) - 1;
		for (uint32_t i = 0; i < size; i++)
			*dstData-- = COLOR_GRAYSCALE_TO_RGB565(*src--);
	} else {
		for (uint32_t i = 0; i < size; i++)
			*dstData++ = COLOR_GRAYSCALE_TO_RGB565(*src++);
	}
}

/*
 * brief Converts the source image pixels from Grayscale to RGB888 and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_Y8ToRGB888(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t size = width * height;

	if (reverse) {
		src += (width * height) - 1;
		dst += (width * height * 3) - 1;
		for (uint32_t i = 0; i < size; i++) {
			uint8_t v = *src--;
			*dst-- = v;
			*dst-- = v;
			*dst-- = v;
		}
	} else {
		for (uint32_t i = 0; i < size; i++) {
			uint8_t v = *src++;
			*dst++ = v;
			*dst++ = v;
			*dst++ = v;
		}
	}
}

/*
 * brief Converts the source image pixels from RGB565 to Binary and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_RGB565ToBinary(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint16_t *srcData = (uint16_t*)src;
	uint32_t *dstData = (uint32_t*)dst;
	uint32_t dstRowLen = (width + UINT32_T_MASK) >> UINT32_T_SHIFT;

	if (reverse) {
		srcData += width * (height - 1);
		dstData += dstRowLen * (height - 1);
		for (uint32_t y = 0; y < height; y++) {
			for (int32_t x = width - 1; x >= 0; x--)
				IMAGE_PUT_BINARY_PIXEL_FAST(dstData, x, COLOR_RGB565_TO_BINARY(srcData[x]));

			srcData -= width;
			dstData -= dstRowLen;
		}
	} else {
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++)
				IMAGE_PUT_BINARY_PIXEL_FAST(dstData, x, COLOR_RGB565_TO_BINARY(srcData[x]));

			srcData += width;
			dstData += dstRowLen;
		}
	}
}

/*
 * brief Converts the source image pixels from RGB565 to Grayscale and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_RGB565ToY8(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t size = width * height;
	uint16_t *srcData = (uint16_t*)src;

	if (reverse) {
		srcData += (width * height) - 1;
		dst += (width * height) - 1;
		for (uint32_t i = 0; i < size; i++)
			*dst-- = COLOR_RGB565_TO_GRAYSCALE(*srcData--);
	} else {
		for (uint32_t i = 0; i < size; i++)
			*dst++ = COLOR_RGB565_TO_GRAYSCALE(*srcData++);
	}
}

/*
 * brief Converts the source image pixels from RGB565 to RGB888 and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_RGB565ToRGB888(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t size = width * height;
	uint16_t *srcData = (uint16_t*)src;

	if (reverse) {
		srcData += (width * height) - 1;
		dst += (width * height * 3) - 1;
		for (uint32_t i = 0; i < size; i++) {
			uint16_t v = *srcData--;
			*dst-- = COLOR_RGB565_TO_R8(v);
			*dst-- = COLOR_RGB565_TO_G8(v);
			*dst-- = COLOR_RGB565_TO_B8(v);
		}
	} else {
		for (uint32_t i = 0; i < size; i++) {
			uint16_t v = *srcData++;
			*dst++ = COLOR_RGB565_TO_B8(v);
			*dst++ = COLOR_RGB565_TO_G8(v);
			*dst++ = COLOR_RGB565_TO_R8(v);
		}
	}
}

/*
 * brief Converts the source image pixels from RGB888 to Binary and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_RGB888ToBinary(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t *dstData = (uint32_t*)dst;
	uint32_t dstRowLen = (width + UINT32_T_MASK) >> UINT32_T_SHIFT;

	if (reverse) {
		src += (width * height * 3) - 1;
		dstData += dstRowLen * (height - 1);
		for (uint32_t y = 0; y < height; y++) {
			for (int32_t x = width - 1; x >= 0; x--) {
				uint8_t r = *src--;
				uint8_t g = *src--;
				uint8_t b = *src--;
				IMAGE_PUT_BINARY_PIXEL_FAST(dstData, x, COLOR_RGB888_TO_BINARY(r, g, b));
			}

			dstData -= dstRowLen;
		}
	} else {
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {
				uint8_t b = *src++;
				uint8_t g = *src++;
				uint8_t r = *src++;
				IMAGE_PUT_BINARY_PIXEL_FAST(dstData, x, COLOR_RGB888_TO_BINARY(r, g, b));
			}

			dstData += dstRowLen;
		}
	}
}

/*
 * brief Converts the source image pixels from RGB888 to Grayscale and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height  Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_RGB888ToY8(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t size = width * height;

	if (reverse) {
		src += (width * height * 3) - 1;
		dst += (width * height) - 1;
		for (uint32_t i = 0; i < size; i++) {
			uint8_t r = *src--;
			uint8_t g = *src--;
			uint8_t b = *src--;
			*dst-- = COLOR_RGB888_TO_Y(r, g, b);
		}
	} else {
		for (uint32_t i = 0; i < size; i++) {
			uint8_t b = *src++;
			uint8_t g = *src++;
			uint8_t r = *src++;
			*dst++ = COLOR_RGB888_TO_Y(r, g, b);
		}
	}
}

/*
 * brief Converts the source image pixels from RGB888 to RGB565 and stores the converted data to the destination.
 * Assuming the two given data pointers point to valid buffers.
 * param src	 Source image data.
 * param dst     Destination image data.
 * param width	 Width of the two images.
 * param height	 Height of the two images.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		 void.
 */
static void STM32Ipl_RGB888ToRGB565(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height, bool reverse)
{
	uint32_t size = width * height;
	uint16_t *dstData = (uint16_t*)dst;

	if (reverse) {
		src += (width * height * 3) - 1;
		dstData += (width * height) - 1;
		for (uint32_t i = 0; i < size; i++) {
			uint8_t r = *src--;
			uint8_t g = *src--;
			uint8_t b = *src--;
			*dstData-- = COLOR_R8_G8_B8_TO_RGB565(r, g, b);
		}
	} else {
		for (uint32_t i = 0; i < size; i++) {
			uint8_t b = *src++;
			uint8_t g = *src++;
			uint8_t r = *src++;
			*dstData++ = COLOR_R8_G8_B8_TO_RGB565(r, g, b);
		}
	}
}

/**
 * brief Converts the source image data to the format of the destination image and stores the
 * converted data to the destination buffer. The two images must have the same resolution.
 * The destination image data buffer must be already allocated and must have the right size to
 * contain the converted image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * param src	  Source image.
 * param dst	  Destination image.
 * param reverse If true, the processing is executed in reverse mode (from the last to the first pixel),
 * otherwise it is executed normally (from the first to the last pixel).
 * return		  stm32ipl_err_Ok on success, error otherwise.
 */
static stm32ipl_err_t STM32Ipl_ConvertRev(const image_t *src, image_t *dst, bool reverse)
{
	if (!src || !dst || !src->data || !dst->data)
		return stm32ipl_err_InvalidParameter;

	if (src->data == dst->data)
		return stm32ipl_err_InvalidParameter;

	if ((src->w != dst->w) || (src->h != dst->h))
		return stm32ipl_err_InvalidParameter;

	switch (src->bpp) {
		case IMAGE_BPP_BINARY:
			switch (dst->bpp) {
				case IMAGE_BPP_BINARY:
					STM32Ipl_SimpleCopy(src->data, dst->data, STM32Ipl_ImageDataSize(dst), reverse);
					break;

				case IMAGE_BPP_GRAYSCALE:
					STM32Ipl_BinaryToY8(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_RGB565:
					STM32Ipl_BinaryToRGB565(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_RGB888:
					STM32Ipl_BinaryToRGB888(src->data, dst->data, src->w, src->h, reverse);
					break;

				default:
					return stm32ipl_err_UnsupportedFormat;
			}
			break;

		case IMAGE_BPP_GRAYSCALE:
			switch (dst->bpp) {
				case IMAGE_BPP_BINARY:
					STM32Ipl_Y8ToBinary(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_GRAYSCALE:
					STM32Ipl_SimpleCopy(src->data, dst->data, STM32Ipl_ImageDataSize(dst), reverse);
					break;

				case IMAGE_BPP_RGB565:
					STM32Ipl_Y8ToRGB565(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_RGB888:
					STM32Ipl_Y8ToRGB888(src->data, dst->data, src->w, src->h, reverse);
					break;

				default:
					return stm32ipl_err_UnsupportedFormat;
			}
			break;

		case IMAGE_BPP_RGB565: {
			switch (dst->bpp) {
				case IMAGE_BPP_BINARY:
					STM32Ipl_RGB565ToBinary(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_GRAYSCALE:
					STM32Ipl_RGB565ToY8(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_RGB565:
					STM32Ipl_SimpleCopy(src->data, dst->data, STM32Ipl_ImageDataSize(dst), reverse);
					break;

				case IMAGE_BPP_RGB888:
					STM32Ipl_RGB565ToRGB888(src->data, dst->data, src->w, src->h, reverse);
					break;

				default:
					return stm32ipl_err_UnsupportedFormat;
			}
			break;
		}

		case IMAGE_BPP_RGB888: {
			switch (dst->bpp) {
				case IMAGE_BPP_BINARY:
					STM32Ipl_RGB888ToBinary(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_GRAYSCALE:
					STM32Ipl_RGB888ToY8(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_RGB565:
					STM32Ipl_RGB888ToRGB565(src->data, dst->data, src->w, src->h, reverse);
					break;

				case IMAGE_BPP_RGB888:
					STM32Ipl_SimpleCopy(src->data, dst->data, STM32Ipl_ImageDataSize(dst), reverse);
					break;

				default:
					return stm32ipl_err_UnsupportedFormat;
			}
			break;
		}

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	return stm32ipl_err_Ok;
}


/**
 * @brief Converts the source image data to the format of the destination image and stores the
 * converted data to the destination buffer. The two images must have the same resolution.
 * The destination image data buffer must be already allocated and must have the right size to
 * contain the converted image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param src	  Source image.
 * @param dst	  Destination image.
 * @return		  stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Convert(const image_t *src, image_t *dst)
{
	return STM32Ipl_ConvertRev(src, dst, false);
}

/**
 * brief Checks if a color format conversion must be performed in reversed mode (starting from the last pixel of the image).
 * param src 	Source image.
 * param dst 	Destination image; its width and height must be greater than zero.
 * return		0 if the convert operation must be performed normally (from the start to the end of the image);
 * 1 if the convert operation must be performed as reversed (from the end to the start of the image);
 * -1 if a reversed convert is not allowed.
 */
static int32_t STM32Ipl_CheckReversedConvert(const image_t *src, const image_t *dst)
{
	int32_t srcSize;
	int32_t dstSize;
	uint32_t srcStart;
	uint32_t dstStart;
	uint32_t srcEnd;
	uint32_t dstEnd;

	if (!src || !dst)
		return -1;

	srcSize = STM32Ipl_ImageDataSize(src);
	dstSize = STM32Ipl_ImageDataSize(dst);
	srcStart = (uint32_t)src->data;
	dstStart = (uint32_t)dst->data;
	srcEnd = srcStart + srcSize - 1;
	dstEnd = dstStart + dstSize - 1;

	if ((dstSize - srcSize) < (dstSize / 3)) {
		return -1;
	} else {
		if (dstEnd <= srcEnd) {
			return 0;
		} else
			if (dstStart >= srcStart) {
				return 1;
			} else {
				return -1;
			}
	}

	return -1;
}

/**
 * @brief Converts the source image data to the format of the destination image and stores the
 * converted data to the destination buffer. The two images must have the same resolution.
 * The destination image data buffer must be already allocated and must have the right size to
 * contain the converted image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * This function supports some particular cases when source and destination data memory buffers overlap.
 * @param src	Source image.
 * @param dst	Destination image.
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_ConvertWithOverlap(const image_t *src, image_t *dst)
{
	int32_t reverse;

	if (!src || !dst || !src->data || !dst->data)
		return stm32ipl_err_InvalidParameter;

	if (src->data == dst->data)
		return stm32ipl_err_InvalidParameter;

	if ((src->w != dst->w) || (src->h != dst->h))
		return stm32ipl_err_InvalidParameter;

	reverse = STM32Ipl_CheckReversedConvert(src, dst);

	if (reverse == -1)
		return stm32ipl_err_NotAllowed;

	return STM32Ipl_ConvertRev(src, dst, reverse);
}

/**
 * @brief Resizes the source image to the destination one with Nearest Neighbor method.
 * The two images must have the same format. The destination data buffer must be allocated by the user
 * and its size must be compatible with resolution and format of the destination image.
 * When specified, roi defines the region of the source image to be scaled to the destination image resolution.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param src 	Source image.
 * @param dst 	Destination image; its width and height must be greater than zero.
 * @param roi	Region of interest of the source image: it must be contained in the source image and have
 * positive dimensions, otherwise an error is returned.
 * @return		stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Resize(const image_t *src, image_t *dst, const rectangle_t *roi)
{
	rectangle_t srcRoi;
	int32_t srcW;
	int32_t srcH;
	int32_t dstW;
	int32_t dstH;
	float wRatio;
	float hRatio;

	if (!src || !dst || !src->data || !src->data)
		return stm32ipl_err_InvalidParameter;

	if ((src->bpp != dst->bpp) || (dst->w < 1) || (dst->h < 1))
		return stm32ipl_err_InvalidParameter;

	srcW = src->w;
	srcH = src->h;
	dstW = dst->w;
	dstH = dst->h;

	STM32Ipl_RectInit(&srcRoi, 0, 0, srcW, srcH);

	if (roi) {
		if (roi->w < 1 || roi->h < 1)
			return stm32ipl_err_WrongROI;

		if (!STM32Ipl_RectContain(&srcRoi, roi))
			return stm32ipl_err_WrongROI;

		STM32Ipl_RectCopy(roi, &srcRoi);
	}

	wRatio = (float)srcRoi.w / dstW;
	hRatio = (float)srcRoi.h / dstH;

	switch (src->bpp) {
		case IMAGE_BPP_BINARY:
			for (uint32_t y = 0; y < dstH; y++) {
				uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) + srcRoi.y);
				uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, y);

				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, x, IMAGE_GET_BINARY_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_GRAYSCALE:
			for (uint32_t y = 0; y < dstH; y++) {
				uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) + srcRoi.y);
				uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, y);

				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, x, IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_RGB565:
			for (uint32_t y = 0; y < dstH; y++) {
				uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) +srcRoi.y);
				uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, y);

				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x, IMAGE_GET_RGB565_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_RGB888:
			for (uint32_t y = 0; y < dstH; y++) {
				rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) + srcRoi.y);
				rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, y);
				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, x, IMAGE_GET_RGB888_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	return stm32ipl_err_Ok;
}

/**
 * brief Checks if a downscale operation must be performed in reversed mode (starting from the last pixel of the image).
 * A reversed resize is allowed for scale-down cases only.
 * param src 	Source image.
 * param dst 	Destination image; its width and height must be greater than zero.
 * return		0 if the resize operation must be performed normally (from the start to the end of the image);
 * 1 if the resize operation must be performed as reversed (from the end to the start of the image);
 * -1 if a reversed resize is not allowed.
 */
static int32_t STM32Ipl_CheckDownscale(const image_t *src, const image_t *dst)
{
	uint32_t srcSize;
	uint32_t dstSize;
	uint32_t srcStart;
	uint32_t dstStart;
	uint32_t srcEnd;
	uint32_t dstEnd;

	if (!src || !dst)
		return -1;

	srcSize = STM32Ipl_ImageDataSize(src);
	dstSize = STM32Ipl_ImageDataSize(dst);
	srcStart = (uint32_t)src->data;
	dstStart = (uint32_t)dst->data;
	srcEnd = srcStart + srcSize - 1;
	dstEnd = dstStart + dstSize - 1;

	if (srcSize < dstSize)
		return -1;
	else
	if ((srcStart >= dstStart) || (srcEnd <= dstStart))
		return 0;
	else
	if ((srcEnd > dstStart) && (dstEnd >= srcEnd))
		return 1;

	return -1;
}

/**
 * @brief Resizes (downscale only) the source image to the destination one with Nearest Neighbor method.
 * The two images must have the same format. The destination data buffer must be allocated by the user
 * and its size must be compatible with resolution and format of the destination image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * This function supports some particular cases when source and destination data memory buffers overlap.
 * Call this function for downscale cases only.
 * @param src 	Source image.
 * @param dst 	Destination image; its width and height must be greater than zero.
 * @return		stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Downscale(const image_t *src, image_t *dst)
{
	int32_t dstW;
	int32_t dstH;
	float wRatio;
	float hRatio;
	int32_t reversed;

	if (!src || !dst || !src->data || !dst->data)
		return stm32ipl_err_InvalidParameter;

	if ((src->bpp != dst->bpp) || (dst->w < 1) || (dst->h < 1))
		return stm32ipl_err_InvalidParameter;

	dstW = dst->w;
	dstH = dst->h;

	wRatio = (float)src->w / dstW;
	hRatio = (float)src->h / dstH;

	reversed = STM32Ipl_CheckDownscale(src, dst);

	if (reversed == -1)
		return stm32ipl_err_NotAllowed;

	if (reversed == 1) {
		switch (src->bpp) {
			case IMAGE_BPP_BINARY:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, x,
								IMAGE_GET_BINARY_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}
				break;

			case IMAGE_BPP_GRAYSCALE:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, x,
								IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}
				break;

			case IMAGE_BPP_RGB565:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, y);

					for (int x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x, IMAGE_GET_RGB565_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}

				break;

			case IMAGE_BPP_RGB888:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, y);

					for (int x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB888_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}
				break;

			default:
				return stm32ipl_err_UnsupportedFormat;
		}
	} else
	if (reversed == 0) {
		switch (src->bpp) {
			case IMAGE_BPP_BINARY:
				for (int32_t y = 0; y < dstH; y++) {
					uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, x,
								IMAGE_GET_BINARY_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}
				break;

			case IMAGE_BPP_GRAYSCALE:
				for (int32_t y = 0; y < dstH; y++) {
					uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, x,
								IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}
				break;

			case IMAGE_BPP_RGB565:
				for (int32_t y = 0; y < dstH; y++) {
					uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x, IMAGE_GET_RGB565_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}

				break;

			case IMAGE_BPP_RGB888:
				for (int32_t y = 0; y < dstH; y++) {
					rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio));
					rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB888_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
				}
				break;

			default:
				return stm32ipl_err_UnsupportedFormat;
		}
	}

	return stm32ipl_err_Ok;
}

/*******************************************************
 *******************************************************
 * TODO: from this point on, check.
 *******************************************************
 *******************************************************/

/**
 * @brief Inverts the image (in-place, no memory allocated; Binary/Grayscale/RGB565 supported).
 * @param img	The image to be inverted
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Invert(image_t *img)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	imlib_invert(img);

	return stm32ipl_err_Ok;
}

/**
 * @brief Histogram equalization (it normalizes contrast and brightness of the image).
 * @param img	The image to be equalized
 * @param mask 	Another image to be used as a pixel level mask for the operation.
 * 			   	The mask must have the same size as the image being operated on.
 * 			   	Only pixels corresponding to the ones set in the mask are modified.
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_HistEq(image_t *img, const image_t *mask)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		if (!mask->data)
			return stm32ipl_err_InvalidParameter;

		if ((mask->h != img->h) || (mask->w != img->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_histeq(img, (image_t*)mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Contrast limited adaptive histogram equalization (it normalizes the contrast and brightness of the image).
 * @param img			The image (Grayscale/RGB565 supported)
 * @param clip_limit 	Provides a way to limit the contrast of the adaptive histogram equalization.
 * 					 	Use a small value, i.e. 10, to produce good equalized images
 * @param mask 			Another image to be used as a pixel level mask for the operation.
 * 			   			The mask should be an image with just black or white pixels and should be
 * 			   			the same size as the image being operated on.
 * 			   			Only pixels corresponding to the ones set in the mask are modified.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ClaheHistEq(image_t *img, float clip_limit, image_t *mask)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		if ((mask->h != img->h) || (mask->w != img->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_clahe_histeq(img, clip_limit, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Standard mean blurring filter using a box filter.
 * @param img		The input image (Binary/Grayscale/RGB565)
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold True enables adaptive thresholding of the image which sets pixels to one or zero based on a pixel’s brightness
 * 					in relation to the brightness of the kernel of pixels around them
 * @param offset	Negative value sets more pixels to 1 as you make it more negative while a positive value only sets the sharpest
 * 					contrast changes to 1
 * @param invert	True invert the binary image resulting output
 * @param mask		Another image to use as a pixel level mask for the operation.
 * 			   		The mask should be an image with just black or white pixels and should be
 * 			   		the same size as the image being operated on.
 * 			   		Only pixels set in the mask are modified.
 * @return			stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_MeanFilter(image_t *img, const int32_t ksize, bool threshold, int32_t offset, bool invert,
		image_t *mask)
{

	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;
	if (mask != NULL) {
		if ((mask->h != img->h) || (mask->w != img->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_mean_filter(img, ksize, threshold, offset, invert, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 *
 * @param img		 Input image (Binary/Grayscale/RGB565)
 * @param ksize		 Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param percentile Controls the percentile of the value used in the kernel.
 * 					 By default each pixel is replaced with the 50th percentile (center) of its neighbors.
 * 					 You can set this to 0 for a min filter, 0.25 for a lower quartile filter, 0.75 for an upper quartile filter, and 1.0 for a max filter.
 * @param threshold  True enable adaptive thresholding of the image which sets pixels to one or zero based on a pixel’s brightness
 * 					 in relation to the brightness of the kernel of pixels around them
 * @param offset	 Negative value sets more pixels to 1 as you make it more negative while a positive value only sets the sharpest
 * 					 contrast changes to 1
 * @param invert	 True invert the binary image resulting output
 * @param mask		 Another image to use as a pixel level mask for the operation.
 * 			   		 The mask should be an image with just black or white pixels and should be
 * 			   		 the same size as the image being operated on.
 * 			   		 Only pixels set in the mask are modified.
 * @return			 stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_MedianFilter(image_t *img, int ksize, float percentile, bool threshold, int offset, bool invert,
		image_t *mask)
{

	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;
	if (mask != NULL) {
		if ((mask->h != img->h) || (mask->w != img->w))
			return stm32ipl_err_InvalidParameter;
	}

	if (((0 <= percentile) && (percentile <= 1)) != true) // 0.5f default
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_median_filter(img, ksize, percentile, threshold, offset, invert, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/*
 *
 *
 * Object Detection
 *
 *
 * */

/**
 * @brief Load a cascade from memory
 * @param cascade 	Cascade struct pointer
 * @param memory  	Cascade memory pointer
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_LoadCascadeFromMemory(cascade_t *cascade, const uint8_t *memory)
{
	int i;
	uint8_t *mem = (uint8_t*)memory;

	/* Check arguments. */
	if (!cascade || !memory)
		return stm32ipl_err_InvalidParameter;

	memset(cascade, 0, sizeof(cascade_t));

	/* read detection window size */
	// read_data(&fp, &cascade->window, sizeof(cascade->window));
	memcpy(&cascade->window, mem, sizeof(cascade->window));
	mem += sizeof(cascade->window);

	/* read num stages */
	// read_data(&fp, &cascade->n_stages, sizeof(cascade->n_stages));
	memcpy(&cascade->n_stages, mem, sizeof(cascade->n_stages));
	mem += sizeof(cascade->n_stages);

	/* read num features in each stages */
	// read_data(&fp, cascade->stages_array, sizeof(uint8_t) * cascade->n_stages);
	cascade->stages_array = mem;
	mem += (sizeof(uint8_t) * cascade->n_stages);

	/* sum num of features in each stages*/
	for (i = 0, cascade->n_features = 0; i < cascade->n_stages; i++) {
		cascade->n_features += cascade->stages_array[i];
	}

	/* read stages thresholds */
	// read_data(&fp, cascade->stages_thresh_array, sizeof(int16_t)*cascade->n_stages);
	cascade->stages_thresh_array = (int16_t*)mem;
	mem += (sizeof(int16_t) * cascade->n_stages);

	/* read features thresholds */
	// read_data(&fp, cascade->tree_thresh_array, sizeof(*cascade->tree_thresh_array)*cascade->n_features);
	cascade->tree_thresh_array = (int16_t*)mem;
	mem += (sizeof(*cascade->tree_thresh_array) * cascade->n_features);

	/* read alpha 1 */
	// read_data(&fp, cascade->alpha1_array, sizeof(*cascade->alpha1_array)*cascade->n_features);
	cascade->alpha1_array = (int16_t*)mem;
	mem += (sizeof(*cascade->alpha1_array) * cascade->n_features);

	/* read alpha 2 */
	// read_data(&fp, cascade->alpha2_array, sizeof(*cascade->alpha2_array)*cascade->n_features);
	cascade->alpha2_array = (int16_t*)mem;
	mem += (sizeof(*cascade->alpha2_array) * cascade->n_features);

	/* read num rectangles per feature*/
	// read_data(&fp, cascade->num_rectangles_array, sizeof(*cascade->num_rectangles_array)*cascade->n_features);
	cascade->num_rectangles_array = (int8_t*)mem;
	mem += (sizeof(*cascade->num_rectangles_array) * cascade->n_features);

	/* sum num of recatngles per feature*/
	for (i = 0, cascade->n_rectangles = 0; i < cascade->n_features; i++) {
		cascade->n_rectangles += cascade->num_rectangles_array[i];
	}

	/* read rectangles weights */
	// read_data(&fp, cascade->weights_array, sizeof(*cascade->weights_array)*cascade->n_rectangles);
	cascade->weights_array = (int8_t*)mem;
	mem += (sizeof(*cascade->weights_array) * cascade->n_rectangles);

	/* read rectangles num rectangles * 4 points */
	//read_data(&fp, cascade->rectangles_array, sizeof(*cascade->rectangles_array)*cascade->n_rectangles *4);
	cascade->rectangles_array = (int8_t*)mem;

	return stm32ipl_err_Ok;
}

/**
 * @brief Load face frontal cascade
 * @param cascade 	Cascade struct pointer
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_LoadFaceCascade(cascade_t *cascade)
{
	imlib_load_cascade(cascade, "frontalface");
	return stm32ipl_err_Ok;
}

/**
 * @brief Load eye cascade
 * @param cascade 	Cascade struct pointer
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_LoadEyeCascade(cascade_t *cascade)
{
	imlib_load_cascade(cascade, "eye");
	return stm32ipl_err_Ok;
}

/**
 * @brief Detect objects, described by cascade, in an image
 * @param cascade		Cascade struct pointer
 * @param img			Input image (Grayscale/RGB565)
 * @param roi			Region of interest, NULL extends the search to the entire image
 * @param scale_factor	Tune the capability to detect objects at different scale ( must be > 1.0 )
 * @param threshold		Tune the detection rate against the false positive rate (0.0 - 1.0)
 * @return an array of bounding boxes ( rectangle_t ) one for each object detected, the array MUST be deallocated by the caller
 */
array_t* STM32Ipl_DetectObject(cascade_t *cascade, const image_t *img, rectangle_t *roi, float scale_factor,
		float threshold)
{
	rectangle_t _roi = { 0, 0, 0, 0 };
	if (roi == NULL) {
		_roi.w = img->w;
		_roi.h = img->h;
	}

	cascade->scale_factor = scale_factor;
	cascade->threshold = threshold;

	fb_alloc_mark();
	array_t *objects_array = imlib_detect_objects((image_t*)img, cascade, (roi == NULL) ? &_roi : roi);
	fb_alloc_free_till_mark();

	return objects_array;
}

/**
 * @brief Sets each pixel in the dst_img to black or white depending on if the pixel in src_img is inside of a threshold
 * @param src			Source image (Binary/Grayscale/RGB565)
 * @param dst			Destination image (Binary/Grayscale/RGB565) with data memory already allocated
 * @param thresholds	List of color_thresholds_list_lnk_data_t. For Grayscale images use LMin, LMax
 * 						and for RGB565 use LMin, LMax, AMin, AMax, BMin, BMax (Threshold in LAB color space)
 * @param invert		Inverts the thresholding operation such that instead of matching pixels inside of some known color bounds pixels
 * 						are matched that are outside of the known color bounds.
 * @param zero			True to instead zero thresholded pixels and leave pixels not in the threshold list untouched.
 * @param mask			Another image to use as a pixel level mask for the operation.
 * 			   			The mask should be an image with just black or white pixels and should be
 * 			   			the same size as the image being operated on.
 * 			   			Only pixels set in the mask are modified.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Binary(image_t *src, image_t *dst, list_t *thresholds, bool invert, bool zero, image_t *mask)
{
	if (!src || !src->data || !dst || !src->data)
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		if ((mask->h != src->h) || (mask->w != src->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_binary(dst, src, thresholds, invert, zero, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Corrects perspective issues in the image by doing a 3D rotation
 * @param img			Input image (Binary/Grayscale/RGB565)
 * @param x_rotation	Number of degrees to rotation the image around the x axis (i.e. this spins the image up and down).
 * @param y_rotation	Number of degrees to rotation the image around the y axis (i.e. this spins the image left and right).
 * @param z_rotation	Number of degrees to rotation the image around the z axis (i.e. this spins the image in place).
 * @param x_translation	Number of units to move the image to the left or right after rotation.
 * 						Because this translation is applied in 3D space the units aren’t pixels.
 * @param y_translation	Number of units to move the image to the up or down after rotation.
 * 						Because this translation is applied in 3D space the units aren’t pixels.
 * @param zoom			Zoom ratio. 1.0 by default.
 * @param fov			FOV must be > 0 and < 180. Is the field-of-view to use internally when doing 2D->3D projection before rotating the image in 3D space.
 * 						As this value approaches 0 the image is placed at infinity away from the viewport.
 * 						As this value approaches 180 the image is placed within the viewport.
 * 						Typically, you set to 60 and not change this value but you can modify it to change the 2D->3D mapping effect.
 *
 * @param corners		Is an array[8], of four (x,y) tuples representing four corners
 * 						used to create a 4-point correspondence homography that will map the
 * 						first corner to (0, 0), the second corner to (image_width-1, 0),
 * 						the third corner to (image_width-1, image_height-1),
 * 						and the fourth corner to (0, image_height-1).
 * 						The 3D rotation is then applied after the image is re-mapped
 *
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation(image_t *img, float x_rotation, float y_rotation, float z_rotation,
		float x_translation, float y_translation, float zoom, float fov, float *corners)
{
	if (img == NULL)
		return stm32ipl_err_InvalidParameter;

	if (fov <= 0 || fov >= 180)
		return stm32ipl_err_InvalidParameter;
	if (zoom <= 0)
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_rotation_corr(img, x_rotation, y_rotation, z_rotation, x_translation, y_translation, zoom, fov, corners);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/*
 *
 *
 * Flip and Rotation
 *
 *
 */

stm32ipl_err_t STM32Ipl_Replace(image_t *src_img, image_t *dst_img, bool hmirror, bool vflip, bool transpose,
		image_t *mask)
{
	// Check arguments.
	if (!src_img || !dst_img || !src_img->data || !dst_img->data)
		return stm32ipl_err_InvalidParameter;

	if (!IM_EQUAL(src_img, dst_img))
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_replace(src_img, NULL, dst_img, 0, hmirror, vflip, transpose, mask);
	fb_alloc_free_till_mark();
	return stm32ipl_err_Ok;

}

/**
 * @brief Flip image (Vertically)
 * @param src_img	Source image (Binary/Grayscale/RGB565)
 * @param dst_img	Destination image ( allocated by the user )
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Flip(image_t *src_img, image_t *dst_img)
{
	return STM32Ipl_Replace(src_img, dst_img, false, true, false, NULL);
}

/**
 * @brief Mirror Image (Horizontally)
 * @param src_img	Source image (Binary/Grayscale/RGB565)
 * @param dst_img	Destination image ( allocated by the user )
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Mirror(image_t *src_img, image_t *dst_img)
{
	return STM32Ipl_Replace(src_img, dst_img, true, false, false, NULL);
}

/**
 * @brief Rotation by 90 degree
 * @param src_img	Source image (Binary/Grayscale/RGB565)
 * @param dst_img	Destination image ( allocated by the user )
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation90(image_t *src_img, image_t *dst_img)
{
	return STM32Ipl_Replace(src_img, dst_img, false, true, true, NULL);
}

/**
 * @brief Rotation by 180 degree
 * @param src_img	Source image ( Binary/Grayscale/RGB565 )
 * @param dst_img	Destination image ( allocated by the user )
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation180(image_t *src_img, image_t *dst_img)
{
	return STM32Ipl_Replace(src_img, dst_img, true, true, false, NULL);
}

/**
 * @brief Rotation by 270°
 * @param src_img	Source image ( Binary/Grayscale/RGB565 )
 * @param dst_img	Destination image ( allocated by the user )
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation270(image_t *src_img, image_t *dst_img)
{
	return STM32Ipl_Replace(src_img, dst_img, true, false, true, NULL);

}

/**
 * @brief Flip and Mirror the image is equal to Rotate image by 180 degree
 * @param src_img	Source image ( Binary/Grayscale/RGB565 )
 * @param dst_img	Destination image ( allocated by the user )
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_FlipMirror(image_t *src_img, image_t *dst_img)
{
	return STM32Ipl_Rotation180(src_img, dst_img);
}

/**
 * @brief Convolves the image by a smoothing gaussian kernel
 * @param img		Input image ( Binary/Grayscale/RGB565 )
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold	if True is enabled adaptive thresholding of the image which sets pixels to one or zero
 * based on a pixel’s brightness in relation to the brightness of the kernel of pixels around them
 * @param unsharp	True improves image sharpness on edges
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Gaussian(image_t *img, uint8_t ksize, bool threshold, bool unsharp)
{

	// Check arguments.
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	int k_2 = ksize * 2;
	int n = k_2 + 1;

	fb_alloc_mark();

	int *pascal = fb_alloc(n * sizeof(int), FB_ALLOC_NO_HINT);
	if (pascal == NULL) {
		fb_alloc_free_till_mark();
		return stm32ipl_err_OutOfMemory;
	}

	pascal[0] = 1;

	for (int i = 0; i < k_2; i++) { // Compute a row of pascal's triangle.
		pascal[i + 1] = (pascal[i] * (k_2 - i)) / (i + 1);
	}

	int *krn = fb_alloc(n * n * sizeof(int), FB_ALLOC_NO_HINT);

	if (krn == NULL) {
		fb_alloc_free_till_mark();
		return stm32ipl_err_OutOfMemory;
	}

	int m = 0;

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			int temp = pascal[i] * pascal[j];
			krn[(i * n) + j] = temp;
			m += temp;
		}
	}

	if (unsharp) {
		krn[((n / 2) * n) + (n / 2)] -= m * 2;
		m = -m;
	}

	/* CMARCH: not necessary.
	 float mul = 1.0f / m;
	 float add = 0.0f;
	 int offset = 0;
	 bool invert = false;
	 */

	// CMARCH imlib_morph(img, ksize, krn, mul, add, threshold, offset, invert, NULL);
	imlib_morph(img, ksize, krn, 1.0f / m, 0, threshold, 0, false, NULL);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Canny edge detector
 * @param grayscale_img 	Input image ( Grayscale )
 * @param min_threshold 	Minimum threshold for hysteresis
 * @param max_threshold 	Maximum threshold for hysteresis
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_EdgeCanny(image_t *grayscale_img, uint8_t min_threshold, uint8_t max_threshold)
{
	// Check arguments.
	if (!grayscale_img || !grayscale_img->data)
		return stm32ipl_err_InvalidParameter;

	if (grayscale_img->bpp != 1)
		return stm32ipl_err_InvalidParameter;

	rectangle_t roi;
	roi.x = 0;
	roi.y = 0;
	roi.w = grayscale_img->w;
	roi.h = grayscale_img->h;

	int thresh[2] = { min_threshold, max_threshold };

	fb_alloc_mark();
	imlib_edge_canny(grayscale_img, &roi, thresh[0], thresh[1]);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Adds pixels to the edges of segmented areas.
 * 		  Convolving a kernel pixels across the image and setting the center pixel of the kernel
 * 		  if the sum of the neighbour pixels set is greater than threshold
 * @param img			Input image ( Binary/Grayscale/RGB565 )
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Minimum value of the sum of neighbour pixel in the kernel
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Dilate(image_t *img, uint8_t ksize, uint8_t threshold)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_dilate(img, ksize, threshold, NULL);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Removes pixels from the edges of segmented areas.
 * 		  Convolving a kernel pixels across the image and zeroing the center pixel of the kernel
 * 		  if the sum of the neighbour pixels set is not greater than threshold.
 * @param img			Input image ( Binary/Grayscale/RGB565 )
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Minimum value of the sum of neighbour pixel in the kernel
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Erode(image_t *img, uint8_t ksize, uint8_t threshold)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_erode(img, ksize, threshold, NULL);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs erosion and dilation on an image.
 * @param img			Input image ( Binary/Grayscale/RGB565 )
 * @param ksize	is 		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Open(image_t *img, uint8_t ksize, uint8_t threshold)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_open(img, ksize, threshold, NULL);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs dilation and erosion on an image in order.
 * @param img			Input image ( Binary/Grayscale/RGB565 )
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Close(image_t *img, uint8_t ksize, uint8_t threshold)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_close(img, ksize, threshold, NULL);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief	Perform the image difference of the image and opened image.
 * @param img			Input image ( Binary/Grayscale/RGB565 )
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @param mask			Another image to use as a pixel level mask for the operation.
 * 			   			The mask should be an image with just black or white pixels and should be
 * 			   			the same size as the image being operated on.
 * 			   			Only pixels set in the mask are modified.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_TopHat(image_t *img, uint8_t ksize, uint8_t threshold, image_t *mask)
{

	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;
	if (mask != NULL) {
		if ((mask->h != img->h) || (mask->w != img->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_top_hat(img, ksize, threshold, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief	Perform the image difference of the image and closed image.
 * @param img			Input image ( Binary/Grayscale/RGB565 )
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @param mask			Another image to use as a pixel level mask for the operation.
 * 			   			The mask should be an image with just black or white pixels and should be
 * 			   			the same size as the image being operated on.
 * 			   			Only pixels set in the mask are modified.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_BlackHat(image_t *img, uint8_t ksize, uint8_t threshold, image_t *mask)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;
	if (mask != NULL) {
		if ((mask->h != img->h) || (mask->w != img->w))
			return stm32ipl_err_InvalidParameter;
	}
	fb_alloc_mark();
	imlib_black_hat(img, ksize, threshold, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief	Finds circles in the image using the Hough transform.
 * @param img 			Input image ( Binary/Grayscale/RGB565 )
 * @param roi 			Region of interest ( NULL for entire image )
 * @param out			List of results. The type of results is find_circles_list_lnk_data_t
 * @param x_stride		The number of x pixels to skip when doing the hough transform. Only increase this if circles you are searching for are large and bulky.
 * @param y_stride		The number of y pixels to skip when doing the hough transform. Only increase this if circles you are searching for are large and bulky.
 * @param threshold		Only circles with a magnitude greater than or equal to threshold are returned
 * @param x_margin		Circles which are x_margin, y_margin, and r_margin pixels apart are merged.
 * @param y_margin		Circles which are x_margin, y_margin, and r_margin pixels apart are merged.
 * @param r_margin		Circles which are x_margin, y_margin, and r_margin pixels apart are merged.
 * @param r_min			Controls the minimum circle radius detected. Increase this to speed up the algorithm
 * @param r_max			Controls the maximum circle radius detected. Decrease this to speed up the algorithm
 * @param r_step		Controls how to step the radius detection by.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_FindCircles(image_t *img, rectangle_t *roi, list_t *out, uint32_t x_stride, uint32_t y_stride,
		uint32_t threshold, uint32_t x_margin, uint32_t y_margin, uint32_t r_margin, uint32_t r_min, uint32_t r_max,
		uint32_t r_step)
{
	rectangle_t _roi = { 0, 0, 0, 0 };

	if (img == NULL || out == NULL)
		return stm32ipl_err_InvalidParameter;
	if (x_stride == 0 || y_stride == 0)
		return stm32ipl_err_InvalidParameter;

	if (roi == NULL) {
		_roi.w = img->w;
		_roi.h = img->h;
	}

	r_min = IM_MAX(r_min, 2);
	r_max = IM_MIN(r_max, IM_MIN((roi->w / 2), (roi->h / 2)));

	fb_alloc_mark();
	imlib_find_circles(out, img, (roi == NULL) ? &_roi : roi, x_stride, y_stride, threshold, x_margin, y_margin,
			r_margin, r_min, r_max, r_step);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief Logically ANDs imgA with imgB (A and B). imgA is overwritten with the operation result
 * @param imgA	Input image ( Binary/Grayscale/RGB565 )
 * @param imgB	same as imgA
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_B_and(image_t *imgA, image_t *imgB, image_t *mask)
{
	if (!imgA || !imgA->data || !imgB || !imgB->data)
		return stm32ipl_err_InvalidParameter;

	if (!IM_EQUAL(imgA, imgB))
		return stm32ipl_err_InvalidParameter;

	if (mask != NULL) {
		if ((mask->h != imgA->h) || (mask->w != imgA->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_b_and(imgA, NULL, imgB, 0, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief Logically XNORs imgA with imgB. imgA is overwritten with the operation result
 * @param imgA	Input image ( Binary/Grayscale/RGB565 )
 * @param imgB	same as imgA
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_B_nand(image_t *imgA, image_t *imgB, image_t *mask)
{

	if (!imgA || !imgA->data || !imgB || !imgB->data)
		return stm32ipl_err_InvalidParameter;

	if (!IM_EQUAL(imgA, imgB))
		return stm32ipl_err_InvalidParameter;

	if (mask != NULL) {
		if ((mask->h != imgA->h) || (mask->w != imgA->w))
			return stm32ipl_err_InvalidParameter;
	}

	int default_val = COLOR_R8_G8_B8_TO_RGB565(IM_MAX(IM_MIN(0, COLOR_R8_MAX), COLOR_R8_MIN),
			IM_MAX(IM_MIN(0, COLOR_G8_MAX), COLOR_G8_MIN), IM_MAX(IM_MIN(0, COLOR_B8_MAX), COLOR_B8_MIN));
	switch (imgA->bpp) {
		case IMAGE_BPP_BINARY: {
			default_val = COLOR_RGB565_TO_BINARY(default_val);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			default_val = COLOR_RGB565_TO_GRAYSCALE(default_val);
			break;
		}
		default: {
			break;
		}
	}

	fb_alloc_mark();
	imlib_b_nand(imgA, NULL, imgB, default_val, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief Logically ORs imgA with imgB (A or B). imgA is overwritten with the operation result
 * @param imgA	Input image ( Binary/Grayscale/RGB565 )
 * @param imgB	same as imgA
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_B_or(image_t *imgA, image_t *imgB, image_t *mask)
{
	if (!imgA || !imgA->data || !imgB || !imgB->data)
		return stm32ipl_err_InvalidParameter;

	if (!IM_EQUAL(imgA, imgB))
		return stm32ipl_err_InvalidParameter;

	if (mask != NULL) {
		if ((mask->h != imgA->h) || (mask->w != imgA->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_b_or(imgA, NULL, imgB, 0, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief Logically NORs imgA with imgB ( A or !B). imgA is overwritten with the operation result
 * @param imgA	Input image ( Binary/Grayscale/RGB565 )
 * @param imgB	same as imgA
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_B_nor(image_t *imgA, image_t *imgB, image_t *mask)
{

	if (!imgA || !imgA->data || !imgB || !imgB->data)
		return stm32ipl_err_InvalidParameter;

	if (!IM_EQUAL(imgA, imgB))
		return stm32ipl_err_InvalidParameter;

	if (mask != NULL) {
		if ((mask->h != imgA->h) || (mask->w != imgA->w))
			return stm32ipl_err_InvalidParameter;
	}

	int default_val = COLOR_R8_G8_B8_TO_RGB565(IM_MAX(IM_MIN(0, COLOR_R8_MAX), COLOR_R8_MIN),
			IM_MAX(IM_MIN(0, COLOR_G8_MAX), COLOR_G8_MIN), IM_MAX(IM_MIN(0, COLOR_B8_MAX), COLOR_B8_MIN));
	switch (imgA->bpp) {
		case IMAGE_BPP_BINARY: {
			default_val = COLOR_RGB565_TO_BINARY(default_val);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			default_val = COLOR_RGB565_TO_GRAYSCALE(default_val);
			break;
		}
		default: {
			break;
		}
	}

	fb_alloc_mark();
	imlib_b_nor(imgA, NULL, imgB, default_val, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief Logically XORs imgA with imgB (A xor B). imgA is overwritten with the operation result
 * @param imgA	Input image ( Binary/Grayscale/RGB565 )
 * @param imgB	same as imgA
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_B_xor(image_t *imgA, image_t *imgB, image_t *mask)
{
	if (!imgA || !imgA->data || !imgB || !imgB->data)
		return stm32ipl_err_InvalidParameter;

	if (!IM_EQUAL(imgA, imgB))
		return stm32ipl_err_InvalidParameter;

	if (mask != NULL) {
		if ((mask->h != imgA->h) || (mask->w != imgA->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_b_xor(imgA, NULL, imgB, 0, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * brief Logically XNORs imgA with imgB. imgA is overwritten with the operation result
 * @param imgA	Input image ( Binary/Grayscale/RGB565 )
 * @param imgB	same as imgA
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_B_xnor(image_t *imgA, image_t *imgB, image_t *mask)
{
	if (!imgA || !imgA->data || !imgB || !imgB->data)
		return stm32ipl_err_InvalidParameter;

	if (!IM_EQUAL(imgA, imgB))
		return stm32ipl_err_InvalidParameter;

	if (mask != NULL) {
		if ((mask->h != imgA->h) || (mask->w != imgA->w))
			return stm32ipl_err_InvalidParameter;
	}

	fb_alloc_mark();
	imlib_b_xnor(imgA, NULL, imgB, 0, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

static void STM32Ipl_b_diff_line_op(image_t *img, int line, void *other, void *data, bool vflipped)
{
	image_t *mask = (image_t*)data;

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			uint32_t *data = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, line);

			if (!mask) {
				for (int i = 0, j = IMAGE_BINARY_LINE_LEN(img); i < j; i++) {
					data[i] = (uint32_t)sqrt(pow(data[i], 2) + pow(((uint32_t*)other)[i], 2));
				}
			} else {
				for (int i = 0, j = img->w; i < j; i++) {
					if (image_get_mask_pixel(mask, i, line)) {
						IMAGE_PUT_BINARY_PIXEL_FAST(data, i,
								(uint32_t)sqrt(pow(IMAGE_GET_BINARY_PIXEL_FAST(data, i),2) + pow(IMAGE_GET_BINARY_PIXEL_FAST(((uint32_t *) other), i),2)));
					}
				}
			}
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			uint8_t *data = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, line);

			if (!mask) {
				for (int i = 0, j = IMAGE_GRAYSCALE_LINE_LEN(img); i < j; i++) {
					data[i] = sqrt(pow(data[i], 2) + pow(((uint8_t*)other)[i], 2));
				}
			} else {
				for (int i = 0, j = img->w; i < j; i++) {
					if (image_get_mask_pixel(mask, i, line)) {
						IMAGE_PUT_GRAYSCALE_PIXEL_FAST(data, i,
								sqrt(pow(IMAGE_GET_GRAYSCALE_PIXEL_FAST(data, i),2) + pow(IMAGE_GET_GRAYSCALE_PIXEL_FAST(((uint8_t *) other), i),2)));
					}
				}
			}
			break;
		}
		case IMAGE_BPP_RGB565: {
			uint16_t *data = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, line);
			uint8_t r;
			uint8_t g;
			uint8_t b;

			if (!mask) {
				for (int i = 0, j = IMAGE_RGB565_LINE_LEN(img); i < j; i++) {
					r = sqrt(pow(COLOR_RGB565_TO_R8(data[i]), 2) + pow(COLOR_RGB565_TO_R8(((uint16_t* ) other)[i]), 2));
					g = sqrt(pow(COLOR_RGB565_TO_G8(data[i]), 2) + pow(COLOR_RGB565_TO_G8(((uint16_t* ) other)[i]), 2));
					b = sqrt(pow(COLOR_RGB565_TO_B8(data[i]), 2) + pow(COLOR_RGB565_TO_B8(((uint16_t* ) other)[i]), 2));
					data[i] = COLOR_R8_G8_B8_TO_RGB565(r, g, b);
				}
			} else {
				for (int i = 0, j = img->w; i < j; i++) {
					if (image_get_mask_pixel(mask, i, line)) {
						uint16_t data565 = IMAGE_GET_RGB565_PIXEL_FAST(data, i);
						uint16_t data565_other = IMAGE_GET_RGB565_PIXEL_FAST(((uint16_t* ) other), i);

						r = sqrt(pow(COLOR_RGB565_TO_R8(data565), 2) + pow(COLOR_RGB565_TO_R8(data565_other), 2));
						g = sqrt(pow(COLOR_RGB565_TO_G8(data565), 2) + pow(COLOR_RGB565_TO_G8(data565_other), 2));
						b = sqrt(pow(COLOR_RGB565_TO_B8(data565), 2) + pow(COLOR_RGB565_TO_B8(data565_other), 2));

						IMAGE_PUT_RGB565_PIXEL_FAST(data, i, COLOR_R8_G8_B8_TO_RGB565(r,g,b));
					}
				}
			}
			break;
		}
		default: {
			break;
		}
	}
}

void STM32Ipl_Diff(image_t *img, const char *path, image_t *other, int scalar, image_t *mask)
{
	fb_alloc_mark();
	imlib_image_operation(img, path, other, scalar, STM32Ipl_b_diff_line_op, mask);
	fb_alloc_free_till_mark();
}

/**
 * @brief Convolves the image by a edge detecting laplacian kernel.
 * @param img		Input image ( Binary/Grayscale/RGB565 )
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param sharpen	if true this method will instead sharpen the image. Increase the kernel size then to increase the image sharpness.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Laplacian(image_t *img, uint8_t ksize, bool sharpen)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	int k_2 = ksize * 2;
	int n = k_2 + 1;

	fb_alloc_mark();

	int *pascal = fb_alloc(n * sizeof(int), FB_ALLOC_NO_HINT);
	if (pascal == NULL) {
		fb_alloc_free_till_mark();
		return stm32ipl_err_OutOfMemory;
	}
	pascal[0] = 1;

	for (int i = 0; i < k_2; i++) { // Compute a row of pascal's triangle.
		pascal[i + 1] = (pascal[i] * (k_2 - i)) / (i + 1);
	}

	int *krn = fb_alloc(n * n * sizeof(int), FB_ALLOC_NO_HINT);
	if (krn == NULL) {
		fb_alloc_free_till_mark();
		return stm32ipl_err_OutOfMemory;
	}

	int m = 0;

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			int temp = pascal[i] * pascal[j];
			krn[(i * n) + j] = -temp;
			m += temp;
		}
	}

	krn[((n / 2) * n) + (n / 2)] += m;
	m = krn[((n / 2) * n) + (n / 2)];

	if (sharpen) {
		krn[((n / 2) * n) + (n / 2)] += m;
	}

	/* CMARCH: not necessary.
	 float mul = 1.0f /m;
	 float add = 0.0f;
	 bool threshold = false;
	 int offset = 0;
	 bool invert = false;
	 */

	// CMARCH imlib_morph(img, ksize, krn, mul, add, threshold, offset, invert, NULL);
	imlib_morph(img, ksize, krn, 1.0f / m, 0, false, 0, false, NULL);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by a edge detecting Sobel kernel.
 * @param img		Input image ( Binary/Grayscale/RGB565 )
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param sharpen	if true this method will instead sharpen the image. Increase the kernel size then to increase the image sharpness.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Sobel(image_t *img, uint8_t ksize, bool sharpen)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	int k_2 = ksize * 2;
	int n = k_2 + 1;

	int *pascal = xalloc(n * sizeof(int));
	if (pascal == NULL) {
		return stm32ipl_err_OutOfMemory;
	}
	pascal[0] = 1;

	for (int i = 0; i < k_2; i++) { // Compute a row of pascal's triangle.
		pascal[i + 1] = (pascal[i] * (k_2 - i)) / (i + 1);
	}

	int *krn = xalloc(n * n * sizeof(int));
	if (krn == NULL) {
		xfree(pascal);
		return stm32ipl_err_OutOfMemory;
	}

	int m = 0;

	for (int i = 0; i < n; i++) {
		if (i < (n - 1) / 2) {
			for (int j = 0; j < n; j++) {
				int temp = pascal[i] * pascal[j];
				krn[(i * n) + j] = -temp;
				m += temp;
			}
		} else
			if (i > (n - 1) / 2) {
				for (int j = 0; j < n; j++) {
					int temp = pascal[i] * pascal[j];
					krn[(i * n) + j] = temp;
					m += temp;
				}

			} else
				if (i == (n - 1) / 2) {
					for (int j = 0; j < n; j++) {
						krn[(i * n) + j] = 0;
					}
				}
	}

	if (sharpen) {
		krn[((n / 2) * n) + (n / 2)] += m / 2;
	}

	float mul = 1.0f / m;
	/* CMARCH: not necessary.
	 float add = 0.0f;
	 bool threshold = false;
	 int offset = 0;
	 bool invert = false;
	 */

	image_t sobel_x;
	image_t sobel_y;

	sobel_x.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (sobel_x.data == NULL) {
		xfree(pascal);
		xfree(krn);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&sobel_x, img->w, img->h, img->bpp, sobel_x.data);

	sobel_y.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (sobel_y.data == NULL) {
		xfree(pascal);
		xfree(krn);
		xfree(sobel_x.data);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&sobel_y, img->w, img->h, img->bpp, sobel_y.data);

	memcpy(sobel_x.data, img->data, STM32Ipl_ImageDataSize(img));
	memcpy(sobel_y.data, img->data, STM32Ipl_ImageDataSize(img));

	fb_alloc_mark();
	// CMARCH imlib_morph(&sobel_x, ksize, krn, mul, add, threshold, offset, invert, NULL);
	imlib_morph(&sobel_x, ksize, krn, mul, 0, false, 0, false, NULL);
	fb_alloc_free_till_mark();

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			int temp = pascal[i] * pascal[j];
			if (j < (n - 1) / 2)
				krn[(i * n) + j] = -temp;
			else
				if (j > (n - 1) / 2)
					krn[(i * n) + j] = temp;
				else
					krn[(i * n) + j] = 0;
		}
	}

	if (sharpen) {
		krn[((n / 2) * n) + (n / 2)] += m % 2 ? m / 2 : (m / 2) + 1;
	}

	fb_alloc_mark();
	// CMARCH imlib_morph(&sobel_y, ksize, krn, mul, add, threshold, offset, invert, NULL);
	imlib_morph(&sobel_y, ksize, krn, mul, 0, false, 0, false, NULL);
	fb_alloc_free_till_mark();

	STM32Ipl_Diff(&sobel_x, NULL, &sobel_y, 1, NULL);

	xfree(img->data);
	STM32Ipl_Init(img, img->w, img->h, sobel_x.bpp, sobel_x.data);
	xfree(sobel_y.data);
	xfree(pascal);
	xfree(krn);

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by a edge detecting Scharr kernel.
 * @param img		Input image ( Binary/Grayscale/RGB565 )
 * @param ksize		Kernel size. Use 1 (3x3 kernel)
 * @param sharpen	if true this method will instead sharpen the image. Increase the kernel size then to increase the image sharpness.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Scharr(image_t *img, uint8_t ksize, bool sharpen)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	int k_2 = ksize * 2;

	if (k_2 != 2)
		return stm32ipl_err_NotImplemented;

	int n = k_2 + 1;

	int *krn = xalloc(n * n * sizeof(int));
	if (krn == NULL) {
		return stm32ipl_err_OutOfMemory;
	}

	int m = 0;

	krn[0] = -3;
	krn[1] = -10;
	krn[2] = -3;
	krn[3] = 0;
	krn[4] = 0;
	krn[5] = 0;
	krn[6] = 3;
	krn[7] = 10;
	krn[8] = 3;

	m = 32;

	if (sharpen) {
		krn[((n / 2) * n) + (n / 2)] += m % 2 ? m / 2 : (m / 2) + 1;
	}

	float mul = 1.0f / m;
	/* CMARCH: not necessary.
	 float add = 0.0f;
	 bool threshold = false;
	 int offset = 0;
	 bool invert = false;
	 */

	image_t scharr_x;
	image_t scharr_y;

	scharr_x.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (scharr_x.data == NULL) {
		xfree(krn);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&scharr_x, img->w, img->h, img->bpp, scharr_x.data);

	scharr_y.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (scharr_y.data == NULL) {
		xfree(krn);
		xfree(scharr_x.data);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&scharr_y, img->w, img->h, img->bpp, scharr_y.data);

	memcpy(scharr_x.data, img->data, STM32Ipl_ImageDataSize(img));
	memcpy(scharr_y.data, img->data, STM32Ipl_ImageDataSize(img));

	if (sharpen) {
		krn[((n / 2) * n) + (n / 2)] += m / 2;
	}

	fb_alloc_mark();
	// CMARCH imlib_morph(&scharr_x, ksize, krn, mul, add, threshold, offset, invert, NULL);
	imlib_morph(&scharr_x, ksize, krn, mul, 0, false, 0, false, NULL);
	fb_alloc_free_till_mark();

	krn[0] = -3;
	krn[1] = 0;
	krn[2] = 3;
	krn[3] = -10;
	krn[4] = 0;
	krn[5] = 10;
	krn[6] = -3;
	krn[7] = 0;
	krn[8] = 3;

	fb_alloc_mark();
	// CMARCH imlib_morph(&scharr_y, ksize, krn, mul, add, threshold, offset, invert, NULL);
	imlib_morph(&scharr_y, ksize, krn, mul, 0, false, 0, false, NULL);
	fb_alloc_free_till_mark();

	STM32Ipl_Diff(&scharr_x, NULL, &scharr_y, 1, NULL);

	xfree(img->data);
	STM32Ipl_Init(img, img->w, img->h, scharr_x.bpp, scharr_x.data);
	xfree(scharr_y.data);
	xfree(krn);

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by krn kernel.
 * @param img		Input image ( Binary/Grayscale/RGB565 )
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param krn		Kernel data
 * @param mul		is number to multiply the convolution pixel results by. If 0 the value that will prevent scaling in the convolution output.
 * 					Basically allows you to do a global contrast adjustment. Pixels that go outside of the image mins and maxes for color channels will be clipped.
 * @param add		is a value to add to each convolution pixel result.
 * 					Basically allows you to do a global brightness adjustment. Pixels that go outside of the image mins and maxes for color channels will be clipped.
 * @param threshold if True the adaptive thresholding of the image is enabled and sets pixels to one or zero based on a pixel’s brightness in relation to the brightness of the kernel of pixels around them.
 * @param offset	negative offset value sets more pixels to 1 as you make it more negative while a positive value only sets the sharpest contrast changes to 1
 * @param invert	if true invert the binary image resulting output
 * @param mask		is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Morph(image_t *img, int ksize, int *krn, float mul, /* CMARCH float*/
int add, bool threshold, int offset, bool invert, image_t *mask)
{
	if (!img || !img->data || !krn)
		return stm32ipl_err_InvalidParameter;

	int n = ksize * 2 + 1;
	int m = 0;

	for (int i = 0; i < n * n; i++) {
		m += krn[i];
	}

	if (m == 0) {
		m = 1;
	}
	if (mul == 0)
		mul = 1.0f / m;

	fb_alloc_mark();
	imlib_morph(img, ksize, krn, mul, add, threshold, offset, invert, mask);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}

/**
 * @brief Create a histogram to have some number of bins that charaterized the img.
 * 		  The Grayscale histograms have one channel with some number of bins.
 * 		  The RGB565 histograms have three channels with some number of bins each.
 * 		  All bins are normalized so that all bins in a channel sum to 1
 * @param img	( Grayscale / RGB565 )
 * @param hist	LAB Histogram
 * @param roi	if not NULL the histogram are contextualized into a region of interest
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GetHistogram(image_t *img, histogram_t *hist, rectangle_t *roi)
{
	if (!img || !img->data || !hist)
		return stm32ipl_err_InvalidParameter;

	list_t thresholds;
	list_init(&thresholds, sizeof(color_thresholds_list_lnk_data_t));
	bool invert = false;
	image_t *other = NULL;

	rectangle_t _roi = { 0, 0, 0, 0 };
	if (roi == NULL) {
		_roi.w = img->w;
		_roi.h = img->h;
	}

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			int bins = COLOR_BINARY_MAX - COLOR_BINARY_MIN + 1;
			if (bins < 2)
				return stm32ipl_err_InvalidParameter;
			hist->LBinCount = bins;
			if (hist->LBinCount < 2)
				return stm32ipl_err_InvalidParameter;

			hist->ABinCount = 0;
			hist->BBinCount = 0;
			hist->LBins = xalloc(hist->LBinCount * sizeof(float));

			if (hist->LBins == NULL) {
				list_free(&thresholds);
				return stm32ipl_err_OutOfMemory;
			}

			hist->ABins = NULL;
			hist->BBins = NULL;
			imlib_get_histogram(hist, img, (roi == NULL) ? &_roi : roi, &thresholds, invert, other);
			list_free(&thresholds);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {

			int bins = COLOR_GRAYSCALE_MAX - COLOR_GRAYSCALE_MIN + 1;
			if (bins < 2)
				return stm32ipl_err_InvalidParameter;
			hist->LBinCount = bins;
			if (hist->LBinCount < 2)
				return stm32ipl_err_InvalidParameter;

			hist->ABinCount = 0;
			hist->BBinCount = 0;
			hist->LBins = xalloc(hist->LBinCount * sizeof(float));
			if (hist->LBins == NULL) {
				list_free(&thresholds);
				return stm32ipl_err_OutOfMemory;
			}
			hist->ABins = NULL;
			hist->BBins = NULL;
			imlib_get_histogram(hist, img, (roi == NULL) ? &_roi : roi, &thresholds, invert, other);
			list_free(&thresholds);
			break;
		}
		case IMAGE_BPP_RGB565: {

			int l_bins = COLOR_L_MAX - COLOR_L_MIN + 1;
			if (l_bins < 2)
				return stm32ipl_err_InvalidParameter;
			hist->LBinCount = l_bins;
			if (hist->LBinCount < 2)
				return stm32ipl_err_InvalidParameter;

			int a_bins = COLOR_A_MAX - COLOR_A_MIN + 1;
			if (a_bins < 2)
				return stm32ipl_err_InvalidParameter;
			hist->ABinCount = a_bins;
			if (hist->ABinCount < 2)
				return stm32ipl_err_InvalidParameter;

			int b_bins = COLOR_B_MAX - COLOR_B_MIN + 1;
			if (b_bins < 2)
				return stm32ipl_err_InvalidParameter;
			hist->BBinCount = b_bins;
			if (hist->BBinCount < 2)
				return stm32ipl_err_InvalidParameter;

			hist->LBins = xalloc(hist->LBinCount * sizeof(float));
			if (hist->LBins == NULL) {
				list_free(&thresholds);
				return stm32ipl_err_OutOfMemory;
			}

			hist->ABins = fb_alloc(hist->ABinCount * sizeof(float),
			FB_ALLOC_NO_HINT);
			if (hist->ABins == NULL) {
				xfree(hist->LBins);
				list_free(&thresholds);
				return stm32ipl_err_OutOfMemory;
			}
			hist->BBins = fb_alloc(hist->BBinCount * sizeof(float),
			FB_ALLOC_NO_HINT);
			if (hist->BBins == NULL) {
				xfree(hist->LBins);
				xfree(hist->ABins);
				list_free(&thresholds);
				return stm32ipl_err_OutOfMemory;
			}

			imlib_get_histogram(hist, img, (roi == NULL) ? &_roi : roi, &thresholds, invert, other);
			list_free(&thresholds);
			break;
		}
		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Computes the mean, median, mode, standard deviation, min, max, lower quartile,
 * 		  and upper quartile of each color channel (LAB space Color) in the histogram.
 * @param img	Input image ( Binary/Grayscale/RGB565 )
 * @param stats output LAB statistics if Binary or Grayscale only L statistics are modified
 * @param roi	if not NULL the statistics are contextualized into a region of interest
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GetStatistics(image_t *img, statistics_t *stats, rectangle_t *roi)
{
	histogram_t hist;
	stm32ipl_err_t error;

	if (!img || !img->data || !stats)
		return stm32ipl_err_InvalidParameter;

	error = STM32Ipl_GetHistogram(img, &hist, roi);

	if (error != stm32ipl_err_Ok)
		return error;

	imlib_get_statistics(stats, (image_bpp_t)img->bpp, &hist);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			if (hist.LBins != NULL) {
				xfree(hist.LBins);
			}
		}
			break;

		case IMAGE_BPP_GRAYSCALE: {
			if (hist.LBins != NULL) {
				xfree(hist.LBins);
			}
		}
			break;

		case IMAGE_BPP_RGB565: {
			if (hist.LBins != NULL) {
				xfree(hist.LBins);
			}
			if (hist.ABins != NULL) {
				xfree(hist.ABins);
			}
			if (hist.BBins != NULL) {
				xfree(hist.BBins);
			}
		}
			break;
		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Quickly changes the image gamma, contrast, and brightness
 * @param img			Input image ( Binary/Grayscale/RGB565 )
 * @param gamma_val		with values greater than 1.0 makes the image darker in a non-linear manner
 * 						while less than 1.0 makes the image brighter.
 * 						The gamma value is applied to the image by scaling all pixel color channels to be between [0:1)
 * 						and then doing a remapping of pow(pixel, 1/gamma) on all pixels before scaling back.
 * @param contrast		with values greater than 1.0 makes the image brighter in a linear manner
 * 						while less than 1.0 makes the image darker.
 * 						The contrast value is applied to the image by scaling all pixel color channels to be between [0:1)
 * 						and then doing a remapping of pixel * contrast on all pixels before scaling back.
 * @param brightness	with values greater than 0.0 makes the image brighter in a constant manner
 * 						while less than 0.0 makes the image darker.
 * 						The brightness value is applied to the image by scaling all pixel color channels to be between [0:1) and then doing a remapping of pixel + brightness on all pixels before scaling back
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GammaCorr(image_t *img, float gamma_val, float contrast, float brightness)
{
	if (!img || !img->data)
		return stm32ipl_err_InvalidParameter;

	fb_alloc_mark();
	imlib_gamma_corr(img, gamma_val, contrast, brightness);
	fb_alloc_free_till_mark();

	return stm32ipl_err_Ok;
}
