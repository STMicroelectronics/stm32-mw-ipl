/**
 ******************************************************************************
 * @file   stm32ipl.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - main image processing module
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
 *
 ******************************************************************************
 */

#include "stm32ipl.h"
#include "matd.h"

//#define M_PI_2  1.57079632f

/**
 * @brief Initializes the memory manager used by this library.
 * @param memAddr	Address of the memory buffer allocated to STM32IPL for its internal purposes.
 * @param memSize	Size of the memory in bytes.
 * @return			void.
 */
void STM32Ipl_InitLib(void *memAddr, uint32_t memSize)
{
	umm_init(memAddr, memSize);
	fb_init();
}

/**
 * @brief De-initializes the memory manager of this library.
 * @return	void.
 */
void STM32Ipl_DeInitLib(void)
{
	umm_uninit();
}

/**
 * @brief Initializes the image structure with the given arguments.
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

/**
 * @brief Allocates a data memory buffer to contain the image pixels and consequently
 * initializes the given image structure. The size of such buffer depends on given
 * width, height and format. Assuming the input image data pointer is null to avoid
 * memory leakage.
 * @param img		Image
 * @param width		Image width
 * @param height	Image height
 * @param bpp		Image format
 * @return			stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_AllocData(image_t *img, uint32_t width, uint32_t height, image_bpp_t format)
{
	uint8_t *data;

	if (!img)
		return stm32ipl_err_InvalidParameter;

	data = xalloc(STM32Ipl_DataSize(width, height, format));
	if (!data) {
		STM32Ipl_Init(img, 0, 0, (image_bpp_t)0, 0);
		return stm32ipl_err_OutOfMemory;
	}

	img->w = width;
	img->h = height;
	img->bpp = format;
	img->data = data;

	return stm32ipl_err_Ok;
}

/**
 * @brief Allocates a data memory buffer to contain the image pixels and consequently
 * initializes the given destination image structure. The size and format of the
 * destination image are taken from the source image. No data pixel is copied from
 * the source image. Assuming the destination image data pointer is null to avoid
 * memory leakage.
 * @param src		Source image, used as reference for size and format.
 * @param dst		Destination image.
 * @return			stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_AllocDataRef(const image_t *src, image_t *dst)
{
	uint8_t *data;

	STM32IPL_CHECK_VALID_IMAGE(src)

	if (!dst)
		return stm32ipl_err_InvalidParameter;

	data = xalloc(STM32Ipl_DataSize(src->w, src->h, (image_bpp_t)src->bpp));
	if (!data) {
		STM32Ipl_Init(dst, 0, 0, (image_bpp_t)0, 0);
		return stm32ipl_err_OutOfMemory;
	}

	dst->w = src->w;
	dst->h = src->h;
	dst->bpp = src->bpp;
	dst->data = data;

	return stm32ipl_err_Ok;
}

/**
 * @brief Releases the data memory buffer of the image and resets the image structure.
 * @param img	Image.
 * @return		void.
 */
void STM32Ipl_ReleaseData(image_t *img)
{
	if (img) {
		xfree(img->data);
		STM32Ipl_Init(img, 0, 0, (image_bpp_t)0, 0);
	}
}

/**
 * @brief Gets the size of the memory needed to store an image with the given properties.
 * The supported formats are Binary, Grayscale, RGB565, RGB888, Bayer.
 * @param width		Image width.
 * @param height	Image height.
 * @param format	Image format.
 * @return			Size of the image data buffer (bytes), 0 in case of wrong input.
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

/**
 * @brief Gets the size the data buffer of the given image (in bytes).
 * The supported formats are Binary, Grayscale, RGB565, RGB888, Bayer.
 * @param img	Image.
 * @return		Size of the image data buffer (bytes), 0 in case of wrong input.
 */
uint32_t STM32Ipl_ImageDataSize(const image_t *img)
{
	return img ? STM32Ipl_DataSize(img->w, img->h, (image_bpp_t)img->bpp) : 0;
}

/**
 * @brief Checks if the image's format is among the provided formats supported.
 * @param img		Image.
 * @param formats	Supported formats.
 * @return			True if image's format is among the provided formats.
 */
bool STM32Ipl_ImageFormatSupported(const image_t *img, uint32_t formats)
{
	stm32ipl_if_t format;

	switch (img->bpp) {
		case IMAGE_BPP_BINARY:
			format = stm32ipl_if_binary;
			break;

		case IMAGE_BPP_GRAYSCALE:
			format = stm32ipl_if_grayscale;
			break;

		case IMAGE_BPP_RGB565:
			format = stm32ipl_if_rgb565;
			break;

		case IMAGE_BPP_RGB888:
			format = stm32ipl_if_rgb888;
			break;

		default:
			return false;
	}

	return (format & formats);
}

/**
 * @brief Copies the source image into the destination one. Only the image structure is copied,
 * so beware the source image data will be shared with the destination image, as no new memory
 * buffer is allocated.
 * @param src	Source image.
 * @param dst   Destination image (assuming its data pointer is null to avoid memory leakage).
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Copy(const image_t *src, image_t *dst)
{
	STM32IPL_CHECK_VALID_IMAGE(src)
	if (!dst)
		return stm32ipl_err_InvalidParameter;

	memcpy(dst, src, sizeof(image_t));

	return stm32ipl_err_Ok;
}

/**
 * @brief Copies the source image data buffer into the destination one.
 * Only the pixel data is actually copied. Source and destination images must have same resolution and format.
 * The destination image data pointer must refer to a valid memory buffer as no new memory is allocated.
 * @param src	Source image.
 * @param dst   Destination image (assuming its data pointer refers to a valid buffer).
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_CopyData(const image_t *src, image_t *dst)
{
	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_SAME_RESOLUTION(src, dst)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	memcpy(dst->data, src->data, STM32Ipl_ImageDataSize(dst));

	return stm32ipl_err_Ok;
}

/**
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

	STM32IPL_CHECK_VALID_IMAGE(src)
	if (!dst)
		return stm32ipl_err_InvalidParameter;

	size = STM32Ipl_ImageDataSize(src);

	if (dst->data) {
		STM32IPL_CHECK_SAME_RESOLUTION(src, dst)
		STM32IPL_CHECK_SAME_FORMAT(src, dst)
	} else {
		data = xalloc(size);
		if (!data) {
			STM32Ipl_Init(dst, 0, 0, (image_bpp_t)0, 0);
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

/**
 * @brief Sets the image pixels to zero.
 * The supported formats (for image and mask) are Binary, Grayscale, RGB565, RGB888.
 * @param img		Image.
 * @param invert	If false and mask is not NULL, the image's pixels are set to 0 when the
 * 					corresponding mask's pixels are 1, otherwise are set to 0 when mask is not 1.
 * @param mask		If NULL, all pixels are set to 0.
 * @return			stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Zero(image_t *img, bool invert, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)

		imlib_zero(img, (image_t*)mask, invert);
	} else {
		memset(img->data, 0, STM32Ipl_ImageDataSize(img));
	}

	return stm32ipl_err_Ok;
}

// FIXME: meglio cambiare nome?
static int STM32Ipl_ColorToValue(image_t *img, uint32_t color)
{

	int default_val = 0;

	rgb888_t pixel888;

	pixel888.r = (color >> 16) & 0xFF;
	pixel888.g = (color >> 8) & 0xFF;
	pixel888.b = color & 0xFF;

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			default_val = COLOR_RGB888_TO_BINARY(pixel888);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			default_val = COLOR_RGB888_TO_GRAYSCALE(pixel888);
			break;
		}
		case IMAGE_BPP_RGB565: {
			default_val = COLOR_R8_G8_B8_TO_RGB565(pixel888.r, pixel888.g, pixel888.b);
			break;
		}
		case IMAGE_BPP_RGB888: {
			default_val = ((pixel888.r << 16) | (pixel888.g << 8) | (pixel888.b));
			break;
		}
		default: {
			default_val = 0;
			break;
		}
	}

	return default_val;
}

/**
 * @brief Fills the image with the given color.
 * If roi is defined, only the corresponding region is filled.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img	Image.
 * @param value	Value used to fill the image.
 * @param roi	Region of interest of the source image: it must be contained in the source image and have
 * positive dimensions, otherwise an error is returned.
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Fill(image_t *img, uint32_t color, const rectangle_t *roi)
// FIXME: cambia uint32_t color  in stm32ipl_color_t color
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	int value = STM32Ipl_ColorToValue(img, color);

	if (roi) {
		STM32IPL_CHECK_ROI(img, roi)

		for (uint32_t y = roi->y, yy = roi->y + roi->h; y < yy; y++) {
			for (uint32_t x = roi->x, xx = roi->x + roi->w; x < xx; x++) {
				imlib_set_pixel(img, x, y, value);
			}
		}
	} else {
		for (uint32_t y = 0, yy = img->h; y < yy; y++) {
			for (uint32_t x = 0, xx = img->w; x < xx; x++) {
				imlib_set_pixel(img, x, y, value);
			}
		}
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Crops a rectangular region of the source image, starting from the given coordinates, and
 * copies it to the destination image. The size of the cropped region is determined by width and height
 * of the destination image. The two images must have same format. The destination image data
 * buffer must be already allocated by the user. If the region to be cropped falls outside the
 * source image, an error is returned. The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src 	Source image.
 * @param dst 	Destination image.
 * @param x		X-coordinate of the top-left corner of the region within the source image
 * @param y		Y-coordinate of the top-left corner of the region within the source image
 * @return		stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Crop(const image_t *src, image_t *dst, uint32_t x, uint32_t y)
{
	rectangle_t srcRoi;
	int32_t dstW;
	int32_t dstH;

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((dst->w < 1) || (dst->h < 1))
		return stm32ipl_err_InvalidParameter;

	dstW = dst->w;
	dstH = dst->h;

	STM32Ipl_RectInit(&srcRoi, x, y, dstW, dstH);

	STM32IPL_CHECK_ROI(src, &srcRoi)

	switch (src->bpp) {
		case IMAGE_BPP_BINARY:
			for (uint32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, srcY);
				uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, dstY);

				for (uint32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, dstX, IMAGE_GET_BINARY_PIXEL_FAST(srcRow, srcX));
			}
			break;

		case IMAGE_BPP_GRAYSCALE:
			for (uint32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, srcY);
				uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, dstY);

				for (uint32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, dstX, IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, srcX));
			}
			break;

		case IMAGE_BPP_RGB565:
			for (uint32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, srcY);
				uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, dstY);

				for (uint32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, dstX, IMAGE_GET_RGB565_PIXEL_FAST(srcRow, srcX));
			}
			break;

		case IMAGE_BPP_RGB888:
			for (uint32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, srcY);
				rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, dstY);

				for (uint32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, dstX, IMAGE_GET_RGB888_PIXEL_FAST(srcRow, srcX));
			}
			break;

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Resizes the source image (whole or a portion of it) to the destination image with Nearest Neighbor method.
 * The two images must have same format. The destination image data buffer must be already allocated
 * by the user and its size must be large enough to contain the resized pixels.  When specified, roi defines
 * the region of the source image to be scaled to the destination image resolution. If roi is null, the whole
 * source image is resized to the destination size. The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src 	Source image.
 * @param dst 	Destination image; its width and height must be greater than zero.
 * @param roi	Region of interest of the source image; if roi is null, the full source image is taken; if roi is not null,
 * it must be contained in the source image and have positive dimensions, otherwise an error is returned.
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

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((dst->w < 1) || (dst->h < 1))
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

		STM32Ipl_RectCopy((rectangle_t*)roi, &srcRoi);
	}

	wRatio = (float)srcRoi.w / dstW;
	hRatio = (float)srcRoi.h / dstH;

	switch (src->bpp) {
		case IMAGE_BPP_BINARY:
			for (uint32_t y = 0; y < dstH; y++) {
				uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) + srcRoi.y);
				uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, y);

				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, x,
							IMAGE_GET_BINARY_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_GRAYSCALE:
			for (uint32_t y = 0; y < dstH; y++) {
				uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) + srcRoi.y);
				uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, y);

				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, x,
							IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_RGB565:
			for (uint32_t y = 0; y < dstH; y++) {
				uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) + srcRoi.y);
				uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, y);

				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x,
							IMAGE_GET_RGB565_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_RGB888:
			for (uint32_t y = 0; y < dstH; y++) {
				rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, fast_floorf(y * hRatio) + srcRoi.y);
				rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, y);
				for (uint32_t x = 0; x < dstW; x++)
					IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, x,
							IMAGE_GET_RGB888_PIXEL_FAST(srcRow, fast_floorf(x * wRatio) + srcRoi.x));
			}
			break;

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Resizes (downscale only) the source image to the destination image with Nearest Neighbor method.
 * The two images must have the same format. The destination image data buffer must be already allocated
 * by the user and its size must be large enough to contain the resized pixels.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * Use this function for downscale cases only.
 * @param src 	Source image.
 * @param dst 	Destination image; its width and height must be greater than zero.
 * @param reversed false to resize in incremeting order, from start to the end of the image.
 *                 true to resize in decrementing order, from end to start of the image.
 * @return		stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Downscale(const image_t *src, image_t *dst, bool reversed)
{
	int32_t dstW;
	int32_t dstH;
	float wRatio;
	float hRatio;

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((dst->w < 1) || (dst->h < 1))
		return stm32ipl_err_InvalidParameter;

	dstW = dst->w;
	dstH = dst->h;

	wRatio = (float)src->w / dstW;
	hRatio = (float)src->h / dstH;

	if (reversed) {
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
						IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB565_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
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
	} else {
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
						IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB565_PIXEL_FAST(srcRow, fast_floorf(x * wRatio)));
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

/**
 * @brief Inverts (in-place) the image; no memory is allocated.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img	Image to be inverted.
 * @return		stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Invert(image_t *img)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	imlib_invert(img);

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs (in-place) a histogram equalization (normalizes contrast and brightness of the image).
 * The supported formats (for image and mask) are Binary, Grayscale, RGB565, RGB888.
 * @param img	Image to be equalized
 * @param mask 	Image to be used as a pixel level mask for the operation.
 * 			   	The mask must have the same resolution as the source image. Only the source pixels that
 * 			   	have the corresponding mask pixels set are considered. The pointer to the mask can be NULL:
 * 			   	in this case all the source image pixels are considered.
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_HistEq(image_t *img, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	imlib_histeq(img, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs (in-place) contrast limited adaptive histogram equalization (it normalizes
 * the contrast and brightness of the image).
 * The supported formats (for image and mask) are Binary, Grayscale, RGB565, RGB888.
 * @param img		Image to be equalized.
 * @param clipLimit Provides a way to limit the contrast of the adaptive histogram equalization.
 * 					Use a small value, i.e. 10, to produce good equalized images
 * @param mask 		Image to be used as a pixel level mask for the operation.
 * 			   		The mask must have the same resolution as the source image. Only the source pixels that
 * 			   		have the corresponding mask pixels set are considered. The pointer to the mask can be NULL:
 * 			   		in this case all the source image pixels are considered.
 * @return			stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ClaheHistEq(image_t *img, float clipLimit, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	imlib_clahe_histeq(img, clipLimit, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/** FIXME: metti a posto la spiegazione di mask e di zero.
 * @brief Binarizes the source image by comparing the source pixels with the given thresholds and stores the
 * resulting black/white pixels to the destination image. Source and destination images must have same resolution.
 * The destination image must be valid and its data memory already allocated by the caller. The format of the
 * destination image must be binary, otherwise it must have the same format of the source image.
 * The supported formats (for source, destination and mask images) are Binary, Grayscale, RGB565, RGB888.
 * @param src			Source image
 * @param dst			Destination image
 * @param thresholds	List of thresholds expressed in LAB color space. For grayscale images, only the L thresholds
 * 						are used (LMin, LMax); for RGB images, all the LAB thresholds are used (LMin, LMax, AMin, AMax,
 * 						BMin, BMax).
 * @param invert		Inverts the thresholding operation such that, instead of matching pixels inside of the
 * 						given color bounds, pixels are matched outside of the given color bounds.
 * @param zero			if true, the destination image  to instead zero thresholded pixels and leave pixels not in the threshold list untouched.
 * @param mask 			Image to be used as a pixel level mask for the operation.
 * 			   			The mask must have the same resolution as the source image. Only the source pixels that
 * 			   			have the corresponding mask pixels set are considered. The pointer to the mask can be NULL:
 * 			   			in this case all the source image pixels are considered.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Binary(const image_t *src, image_t *dst, list_t *thresholds, bool invert, bool zero,
		const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_RESOLUTION(src, dst)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(src, mask)
	}

	imlib_binary(dst, (image_t*)src, thresholds, invert, zero, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/** FIXME perchè non chiamarla Affine Transformation?
 * @brief Corrects (in-place) perspective issues in the image by doing a 3D rotation.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img			Image to be trasformed.
 * @param rotationX		Number of rotation degrees around the X axis (i.e. this spins the image up and down).
 * @param rotationY		Number of rotation degrees around the Y axis (i.e. this spins the image left and right).
 * @param rotationZ		Number of rotation degrees around the Z axis (i.e. this spins the image in place).
 * @param translationX	Number of units to move the image to the left or right after rotation.
 * 						Because this translation is applied in 3D space the units are not pixels.
 * @param translationY	Number of units to move the image to the up or down after rotation.
 * 						Because this translation is applied in 3D space the units are not pixels.
 * @param zoom			Zoom ratio (1.0f by default).
 * @param fov			FOV must be > 0 and < 180. Used internally when doing 2D->3D projection before rotating the image in 3D space.
 * 						As this value approaches 0, the image is placed at infinity away from the viewport. As this value approaches 180,
 * 						the image is placed within the viewport. Used to change the 2D->3D mapping effect.
 * 						Typical value is 60.
 * @param corners		Pointer to an array of 8 float values, corresponding to four (x,y) tuples representing four corners used to
 * 						create a 4-point correspondence homography that will map the first corner to (0, 0), the second corner to
 * 						(image_width-1, 0), the third corner to (image_width-1, image_height-1), and the fourth corner to (0, image_height-1).
 * 						The 3D rotation is then applied after the image is re-mapped. *
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation(image_t *img, float rotationX, float rotationY, float rotationZ, float translationX,
		float translationY, float zoom, float fov, const float *corners)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if ((fov <= 0) || (fov >= 180) || (zoom <= 0))
		return stm32ipl_err_InvalidParameter;

	imlib_rotation_corr(img, rotationX, rotationY, rotationZ, translationX, translationY, zoom, fov, (float*)corners);

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs lens correction to un-fisheye the image due to the lens distortion.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img		Input image
 * @param strength	defining how much to un-fisheye the image. Try 1.8 and then increase or decrease from there until the image looks good.
 * @param zoom		is the amount to zoom in on the image by. The value must be > 0
 * @param x_corr	floating point pixel offset from center. Can be negative or positive.
 * @param y_corr	floating point pixel offset from center. Can be negative or positive.
 * @return	stm32ipl_err_Ok on success
 * @note width and height must be even numbers
 */
stm32ipl_err_t STM32Ipl_LensCorr(image_t *img, float strength, float zoom, float x_corr, float y_corr)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if ((strength <= 0) || (zoom <= 0) || ((img->w % 2) != 0) || ((img->h % 2) != 0))
		return stm32ipl_err_InvalidParameter;

	imlib_lens_corr(img, strength, zoom, x_corr, y_corr);

	return stm32ipl_err_Ok;
}

/**
 * @brief Transforms the source image into the destination image by using the given transformation parameters. The two images
 * must have same format and resolution. The destination image must be valid and its data memory already allocated by the caller.
 * The supported formats (for source, destination and mask images) are Binary, Grayscale, RGB565, RGB888.
 * @param src		Source image.
 * @param dst		Destination image.
 * @param hmirror	True to horizontally mirror the replacing image.
 * @param vflip		True to vertically flip the replacing image
 * @param transpose	True to flip the image along the diagonal (this changes the image image width/height if the image is non-square).
 * @param mask		Another image to use as a pixel level mask for the operation.
 * 			   		The mask should be an image with just black or white pixels and should be
 * 			   		the same size as the image being operated on. Only pixels set in the mask are modified.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Replace(const image_t *src, image_t *dst, bool mirror, bool flip, bool transpose,
		const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)
	STM32IPL_CHECK_SAME_RESOLUTION(src, dst)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(src, mask)
	}

	imlib_replace((image_t*)src, NULL, dst, 0, mirror, flip, transpose, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Vertically flips the source image into the destination image.
 * The two images must have same format and resolution.
 * The destination image must be valid and its data memory already allocated by the caller.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image
 * @param dst	Destination image
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Flip(const image_t *src, image_t *dst)
{
	return STM32Ipl_Replace((image_t*)src, dst, false, true, false, NULL);
}

/**
 * @brief Horizontally mirrors the source image into the destination image.
 * The two images must have same format and resolution.
 * The destination image must be valid and its data memory already allocated by the caller.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image
 * @param dst	Destination image
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Mirror(const image_t *src, image_t *dst)
{
	return STM32Ipl_Replace((image_t*)src, dst, true, false, false, NULL);
}

/**
 * @brief Flips and mirrors the source image into the destination image (same as a 180 degrees rotation).
 * The two images must have same format and resolution.
 * The destination image must be valid and its data memory already allocated by the caller.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image
 * @param dst	Destination image
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_FlipMirror(const image_t *src, image_t *dst)
{
	return STM32Ipl_Replace((image_t*)src, dst, true, true, false, NULL);
}

/**
 * @brief Rotates (clockwise) the source image by 90 degrees into the destination image.
 * The two images must have same format and resolution.
 * The destination image must be valid and its data memory already allocated by the caller.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image
 * @param dst	Destination image
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation90(const image_t *src, image_t *dst)
{
	return STM32Ipl_Replace((image_t*)src, dst, false, true, true, NULL);
}

/**
 * @brief Rotates (clockwise) the source image by 180 degrees into the destination image.
 * The two images must have same format and resolution.
 * The destination image must be valid and its data memory already allocated by the caller.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image
 * @param dst	Destination image
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation180(const image_t *src, image_t *dst)
{
	return STM32Ipl_Replace((image_t*)src, dst, true, true, false, NULL);
}

/**
 * @brief Rotates (clockwise) the source image by 270 degrees into the destination image.
 * The two images must have same format and resolution.
 * The destination image must be valid and its data memory already allocated by the caller.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image
 * @param dst	Destination image
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Rotation270(const image_t *src, image_t *dst)
{
	return STM32Ipl_Replace((image_t*)src, dst, true, false, true, NULL);

}

/*******************************************************
 *******************************************************
 * TODO: from this point on, check.
 *******************************************************
 *******************************************************/
/////////// ULTIMO

// FIXME: da qui in poi:
// 1) nomi argomenti senza underscore e con maiuscola interna

/**
 * @brief Standard mean blurring filter using a box filter.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
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
		const image_t *mask)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	imlib_mean_filter(img, ksize, threshold, offset, invert, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Standard mean blurring filter using a box filter.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		 Input image
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
		const image_t *mask)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	if (((0 <= percentile) && (percentile <= 1)) != true) // 0.5f default
		return stm32ipl_err_InvalidParameter;

	imlib_median_filter(img, ksize, percentile, threshold, offset, invert, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by a smoothing gaussian kernel
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold	if True is enabled adaptive thresholding of the image which sets pixels to one or zero
 * based on a pixel’s brightness in relation to the brightness of the kernel of pixels around them
 * @param unsharp	True improves image sharpness on edges
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Gaussian(image_t *img, uint8_t ksize, bool threshold, bool unsharp)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	int k_2 = ksize * 2;
	int n = k_2 + 1;

	int *pascal = xalloc0(n * sizeof(int));
	if (pascal == NULL)
		return stm32ipl_err_OutOfMemory;

	pascal[0] = 1;

	for (int i = 0; i < k_2; i++) { // Compute a row of pascal's triangle.
		pascal[i + 1] = (pascal[i] * (k_2 - i)) / (i + 1);
	}

	int *krn = xalloc0(n * n * sizeof(int));

	if (krn == NULL) {
		xfree(pascal);
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

	xfree(pascal);

	if (unsharp) {
		krn[((n / 2) * n) + (n / 2)] -= m * 2;
		m = -m;
	}

	imlib_morph(img, ksize, krn, 1.0f / m, 0, threshold, 0, false, NULL);

	xfree(krn);

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

	STM32IPL_CHECK_VALID_IMAGE(grayscale_img)

	/* Check format. */
	if (grayscale_img->bpp != IMAGE_BPP_GRAYSCALE)
		return stm32ipl_err_UnsupportedFormat;

	rectangle_t roi;
	roi.x = 0;
	roi.y = 0;
	roi.w = grayscale_img->w;
	roi.h = grayscale_img->h;

	int thresh[2] = { min_threshold, max_threshold };

	imlib_edge_canny(grayscale_img, &roi, thresh[0], thresh[1]);

	return stm32ipl_err_Ok;
}

/**
 * @brief Adds pixels to the edges of segmented areas.
 * Convolving a kernel pixels across the previously segmented image and setting the center pixel of the kernel
 * if the sum of the neighbour pixels set is greater than threshold
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img			Input image
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Minimum value of the sum of neighbour pixel in the kernel
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Dilate(image_t *img, uint8_t ksize, uint8_t threshold)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	imlib_dilate(img, ksize, threshold, NULL);

	return stm32ipl_err_Ok;
}

/**
 * @brief Removes pixels from the edges of segmented areas.
 * Convolving a kernel pixels across the image and zeroing the center pixel of the kernel
 * 		  if the sum of the neighbour pixels set is not greater than threshold.
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img			Input image
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Minimum value of the sum of neighbour pixel in the kernel
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Erode(image_t *img, uint8_t ksize, uint8_t threshold)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	imlib_erode(img, ksize, threshold, NULL);

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs erosion and dilation on an image.
 * The supported formats are Binary, Grayscale, RGB565.
 * @param img			Input image
 * @param ksize	is 		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Open(image_t *img, uint8_t ksize, uint8_t threshold)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	imlib_open(img, ksize, threshold, NULL);

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs dilation and erosion on an image in order.
 * The supported formats are Binary, Grayscale, RGB565.
 * @param img			Input image
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Close(image_t *img, uint8_t ksize, uint8_t threshold)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	imlib_close(img, ksize, threshold, NULL);

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs the difference of the image and the opened image.
 * The supported formats are Binary, Grayscale, RGB565.
 * @param img			Input image
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @param mask			Another image to use as a pixel level mask for the operation.
 * 			   			The mask should be an image with just black or white pixels and should be
 * 			   			the same size as the image being operated on.
 * 			   			Only pixels set in the mask are modified.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_TopHat(image_t *img, uint8_t ksize, uint8_t threshold, const image_t *mask)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	imlib_top_hat(img, ksize, threshold, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Performs the difference of the image and the closed image.
 * The supported formats are Binary, Grayscale, RGB565.
 * @param img			Input image
 * @param ksize			Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param threshold 	Threshold parameter used by Dilate and Erode
 * @param mask			Another image to use as a pixel level mask for the operation.
 * 			   			The mask should be an image with just black or white pixels and should be
 * 			   			the same size as the image being operated on.
 * 			   			Only pixels set in the mask are modified.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_BlackHat(image_t *img, uint8_t ksize, uint8_t threshold, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	imlib_black_hat(img, ksize, threshold, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief	Finds circles in the image using the Hough transform.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img 			Input image
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
stm32ipl_err_t STM32Ipl_FindCircles(const image_t *img, const rectangle_t *roi, list_t *out, uint32_t x_stride,
		uint32_t y_stride, uint32_t threshold, uint32_t x_margin, uint32_t y_margin, uint32_t r_margin, uint32_t r_min,
		uint32_t r_max, uint32_t r_step)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

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

	imlib_find_circles(out, (image_t*)img, (roi == NULL) ? &_roi : (rectangle_t*)roi, x_stride, y_stride, threshold,
			x_margin, y_margin, r_margin, r_min, r_max, r_step);

	return stm32ipl_err_Ok;
}

/**
 * brief Logically ANDs imgA with imgB (A and B). imgA is overwritten with the result ot the operation.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param imgA	Input image
 * @param imgB	same imgA format
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_And(image_t *imgA, const image_t *imgB, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(imgA)
	STM32IPL_CHECK_VALID_IMAGE(imgB)
	STM32IPL_CHECK_FORMAT(imgA, STM32IPL_IF_ALL)
	STM32IPL_CHECK_EQUAL(imgA, imgB)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(imgA, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar.

	imlib_b_and(imgA, NULL, (image_t*)imgB, 0, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * brief Logically XNORs imgA with imgB. imgA is overwritten with the operation result
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param imgA	Input image
 * @param imgB	same imgA format and size
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Nand(image_t *imgA, const image_t *imgB, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(imgA)
	STM32IPL_CHECK_VALID_IMAGE(imgB)
	STM32IPL_CHECK_FORMAT(imgA, STM32IPL_IF_ALL)
	STM32IPL_CHECK_EQUAL(imgA, imgB)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(imgA, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	/** This code is if imlib_b_nand(imgA, NULL, NULL, default_val, (image_t*)mask);*/
	/*
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
	 */

	//imlib_b_nand(imgA, NULL, (image_t*)imgB, default_val, (image_t*)mask);
	imlib_b_nand(imgA, NULL, (image_t*)imgB, 0, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * brief Logically ORs imgA with imgB (A or B). imgA is overwritten with the operation result
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param imgA	Input image
 * @param imgB	same imgA format and size
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Or(image_t *imgA, const image_t *imgB, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(imgA)
	STM32IPL_CHECK_VALID_IMAGE(imgB)
	STM32IPL_CHECK_FORMAT(imgA, STM32IPL_IF_ALL)
	STM32IPL_CHECK_EQUAL(imgA, imgB)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(imgA, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_b_or(imgA, NULL, (image_t*)imgB, 0, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * brief Logically NORs imgA with imgB ( A or !B). imgA is overwritten with the operation result
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param imgA	Input image
 * @param imgB	same imgA format and size
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Nor(image_t *imgA, const image_t *imgB, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(imgA)
	STM32IPL_CHECK_VALID_IMAGE(imgB)
	STM32IPL_CHECK_FORMAT(imgA, STM32IPL_IF_ALL)
	STM32IPL_CHECK_EQUAL(imgA, imgB)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(imgA, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	/* FIXME This code is if imlib_b_nor(imgA, NULL, NULL, default_val, (image_t*)mask);*/
	/*	int default_val = COLOR_R8_G8_B8_TO_RGB565(IM_MAX(IM_MIN(0, COLOR_R8_MAX), COLOR_R8_MIN),
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
	 }*/

	//imlib_b_nor(imgA, NULL, (image_t*)imgB, default_val, (image_t*)mask);
	imlib_b_nor(imgA, NULL, (image_t*)imgB, 0, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * brief Logically XORs imgA with imgB (A xor B). imgA is overwritten with the operation result
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param imgA	Input image
 * @param imgB	same imgA format and size
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Xor(image_t *imgA, const image_t *imgB, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(imgA)
	STM32IPL_CHECK_VALID_IMAGE(imgB)
	STM32IPL_CHECK_FORMAT(imgA, STM32IPL_IF_ALL)
	STM32IPL_CHECK_EQUAL(imgA, imgB)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(imgA, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_b_xor(imgA, NULL, (image_t*)imgB, 0, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * brief Logically XNORs imgA with imgB. imgA is overwritten with the operation result
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param imgA	Input image
 * @param imgB	same imgA format and size
 * @param mask	is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Xnor(image_t *imgA, const image_t *imgB, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(imgA)
	STM32IPL_CHECK_VALID_IMAGE(imgB)
	STM32IPL_CHECK_FORMAT(imgA, STM32IPL_IF_ALL)
	STM32IPL_CHECK_EQUAL(imgA, imgB)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(imgA, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_b_xnor(imgA, NULL, (image_t*)imgB, 0, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Adds other image (or a scalar value) to image
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img	 Input image
 * @param other	 same (width, height, bpp) as img or NULL ( NULL is a value supported in the future)
 * @param scalar if other is NULL add scalar to each pixel (AARRGGBB)
 * @param mask	 is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Add(image_t *img, const image_t *other, uint32_t scalar, const image_t *mask)
{
	uint32_t newScalar = scalar;

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	/* Alternatively use the other image or the scalar value to be added to the source image. */
	if (other) {
		STM32IPL_CHECK_VALID_IMAGE(other)
		STM32IPL_CHECK_EQUAL(img, other)
	} else {
		newScalar = STM32Ipl_ColorToValue(img, scalar);
	}

	imlib_add(img, NULL, (image_t*)other, newScalar, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Subtracts an other image pixel-wise (or scalar) to Input image.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img	 Input image
 * @param other	 same (width, height, bpp) as img or NULL
 * @param scalar if other is NULL add scalar to each pixel (AARRGGBB)
 * @param reverse if True img = other-img otherwise img = img-other
 * @param mask	 is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Sub(image_t *img, const image_t *other, uint32_t scalar, bool reverse, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(other)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!IM_EQUAL(img, other)) // STM32IPL_CHECK_SAME_RESOLUTION + STM32IPL_CHECK_SAME_FORMAT
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_sub(img, NULL, (image_t*)other, scalar, reverse, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Multiplies two images pixel-wise with each other.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	 Input image
 * @param other	 same (width, height, bpp) as img or NULL ( NULL is a value supported in the future)
 * @param scalar if other is NULL mul scalar to each pixel
 * @param invert if True the multiplication operation change from a*b to 1/((1/a)*(1/b)). In particular, this lightens the image instead of darkening it (e.g. multiply versus burn operations).
 * @param mask	 is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Mul(image_t *img, const image_t *other, uint32_t scalar, bool reverse, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(other)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!IM_EQUAL(img, other)) // STM32IPL_CHECK_SAME_RESOLUTION + STM32IPL_CHECK_SAME_FORMAT
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_mul(img, NULL, (image_t*)other, scalar, reverse, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Divides this Input Image by another one.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	 Input image
 * @param other	 same (width, height, bpp) as img or NULL ( NULL is a value supported in the future)
 * @param scalar if other is NULL div scalar to each pixel
 * @param reverse if True to change the division direction from a/b to b/a
 * @param mod	 if True to change the division operation to the modulus operation.
 * @param mask	 is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Div(image_t *img, const image_t *other, uint32_t scalar, bool reverse, bool mod, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(other)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!IM_EQUAL(img, other)) // STM32IPL_CHECK_SAME_RESOLUTION + STM32IPL_CHECK_SAME_FORMAT
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_div(img, NULL, (image_t*)other, scalar, reverse, mod, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Returns the minimum values of pixel image of two images pixel-wise.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	 Input image
 * @param other	 same (width, height, bpp) as img or NULL ( NULL is a value supported in the future)
 * @param scalar if other is NULL pixel = min(scalar,pixel)
 * @param mask	 is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Min(image_t *img, const image_t *other, uint32_t scalar, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(other)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!IM_EQUAL(img, other)) // STM32IPL_CHECK_SAME_RESOLUTION + STM32IPL_CHECK_SAME_FORMAT
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_min(img, NULL, (image_t*)other, scalar, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Returns the maximum values of pixel image of two images pixel-wise.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	 Input image
 * @param other	 same (width, height, bpp) as img or NULL ( NULL is a value supported in the future)
 * @param scalar if other is NULL pixel = max(scalar,pixel)
 * @param mask	 is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Max(image_t *img, const image_t *other, uint32_t scalar, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(other)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!IM_EQUAL(img, other)) // STM32IPL_CHECK_SAME_RESOLUTION + STM32IPL_CHECK_SAME_FORMAT
		return stm32ipl_err_InvalidParameter;

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	// FIXME: aggiungi parametro scalar e qui la chiamata alla funzione di conversione dello stesso scalar

	imlib_max(img, NULL, (image_t*)other, scalar, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by a edge detecting laplacian kernel.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param sharpen	if true this method will instead sharpen the image. Increase the kernel size then to increase the image sharpness.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Laplacian(image_t *img, uint8_t ksize, bool sharpen)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	int k_2 = ksize * 2;
	int n = k_2 + 1;

	int *pascal = xalloc0(n * sizeof(int));

	if (pascal == NULL)
		return stm32ipl_err_OutOfMemory;

	pascal[0] = 1;

	for (int i = 0; i < k_2; i++) { // Compute a row of pascal's triangle.
		pascal[i + 1] = (pascal[i] * (k_2 - i)) / (i + 1);
	}

	int *krn = xalloc0(n * n * sizeof(int));
	if (krn == NULL) {
		xfree(pascal);
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

	xfree(pascal);

	krn[((n / 2) * n) + (n / 2)] += m;
	m = krn[((n / 2) * n) + (n / 2)];

	if (sharpen) {
		krn[((n / 2) * n) + (n / 2)] += m;
	}

	imlib_morph(img, ksize, krn, 1.0f / m, 0, false, 0, false, NULL);

	xfree(krn);

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by a edge detecting Sobel kernel.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
 * @param ksize		Kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), ..., n (((n*2)+1)x((n*2)+1) kernel)
 * @param sharpen	if true this method will instead sharpen the image. Increase the kernel size then to increase the image sharpness.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Sobel(image_t *img, uint8_t ksize, bool sharpen)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

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

	image_t sobel_x;
	image_t sobel_y;

	sobel_x.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (sobel_x.data == NULL) {
		xfree(pascal);
		xfree(krn);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&sobel_x, img->w, img->h, (image_bpp_t)img->bpp, (void*)sobel_x.data);

	sobel_y.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (sobel_y.data == NULL) {
		xfree(pascal);
		xfree(krn);
		xfree(sobel_x.data);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&sobel_y, img->w, img->h, (image_bpp_t)img->bpp, (void*)sobel_y.data);

	memcpy(sobel_x.data, img->data, STM32Ipl_ImageDataSize(img));
	memcpy(sobel_y.data, img->data, STM32Ipl_ImageDataSize(img));

	imlib_morph(&sobel_x, ksize, krn, mul, 0, false, 0, false, NULL);

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

	imlib_morph(&sobel_y, ksize, krn, mul, 0, false, 0, false, NULL);

	STM32Ipl_Add(&sobel_x, &sobel_y, 1, NULL);

	xfree(img->data);
	STM32Ipl_Init(img, img->w, img->h, (image_bpp_t)sobel_x.bpp, (void*)sobel_x.data);
	xfree(sobel_y.data);
	xfree(pascal);
	xfree(krn);

	return stm32ipl_err_Ok;
}

/*TODO Sharr for each ksize and not only for ksize == 1*/
/**
 * @brief Convolves the image by a edge detecting Scharr kernel.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
 * @param ksize		Kernel size. Use 1 (3x3 kernel)
 * @param sharpen	if true this method will instead sharpen the image. Increase the kernel size then to increase the image sharpness.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Scharr(image_t *img, uint8_t ksize, bool sharpen)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

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

	image_t scharr_x;
	image_t scharr_y;

	scharr_x.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (scharr_x.data == NULL) {
		xfree(krn);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&scharr_x, img->w, img->h, (image_bpp_t)img->bpp, (void*)scharr_x.data);

	scharr_y.data = xalloc(STM32Ipl_ImageDataSize(img));
	if (scharr_y.data == NULL) {
		xfree(krn);
		xfree(scharr_x.data);
		return stm32ipl_err_OutOfMemory;
	}
	STM32Ipl_Init(&scharr_y, img->w, img->h, (image_bpp_t)img->bpp, (void*)scharr_y.data);

	memcpy(scharr_x.data, img->data, STM32Ipl_ImageDataSize(img));
	memcpy(scharr_y.data, img->data, STM32Ipl_ImageDataSize(img));

	if (sharpen) {
		krn[((n / 2) * n) + (n / 2)] += m / 2;
	}

	imlib_morph(&scharr_x, ksize, krn, mul, 0, false, 0, false, NULL);

	krn[0] = -3;
	krn[1] = 0;
	krn[2] = 3;
	krn[3] = -10;
	krn[4] = 0;
	krn[5] = 10;
	krn[6] = -3;
	krn[7] = 0;
	krn[8] = 3;

	imlib_morph(&scharr_y, ksize, krn, mul, 0, false, 0, false, NULL);

	STM32Ipl_Add(&scharr_x, &scharr_y, 1, NULL);

	xfree(img->data);
	STM32Ipl_Init(img, img->w, img->h, (image_bpp_t)scharr_x.bpp, (void*)scharr_x.data);
	xfree(scharr_y.data);
	xfree(krn);

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by krn kernel.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
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
stm32ipl_err_t STM32Ipl_Morph(image_t *img, int ksize, int *krn, float mul, int add, bool threshold, int offset,
bool invert, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

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

	imlib_morph(img, ksize, krn, mul, add, threshold, offset, invert, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Create a histogram to have some number of bins that charaterized the img.
 * 		  The Grayscale histograms have one channel with some number of bins.
 * 		  The RGB565 histograms have three channels with some number of bins each.
 * 		  All bins are normalized so that all bins in a channel sum to 1
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img	Input Image
 * @param hist	LAB Histogram
 * @param roi	if not NULL the histogram are contextualized into a region of interest
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GetHistogram(const image_t *img, histogram_t *hist, const rectangle_t *roi)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

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
				return stm32ipl_err_OutOfMemory;
			}

			hist->ABins = NULL;
			hist->BBins = NULL;
			imlib_get_histogram(hist, (image_t*)img, (roi == NULL) ? &_roi : (rectangle_t*)roi, &thresholds, invert,
					other);
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
				return stm32ipl_err_OutOfMemory;
			}
			hist->ABins = NULL;
			hist->BBins = NULL;
			imlib_get_histogram(hist, (image_t*)img, (roi == NULL) ? &_roi : (rectangle_t*)roi, &thresholds, invert,
					other);
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
				return stm32ipl_err_OutOfMemory;
			}

			hist->ABins = xalloc(hist->ABinCount * sizeof(float));
			if (hist->ABins == NULL) {
				xfree(hist->LBins);
				return stm32ipl_err_OutOfMemory;
			}
			hist->BBins = xalloc(hist->BBinCount * sizeof(float));
			if (hist->BBins == NULL) {
				xfree(hist->LBins);
				xfree(hist->ABins);
				return stm32ipl_err_OutOfMemory;
			}

			imlib_get_histogram(hist, (image_t*)img, (roi == NULL) ? &_roi : (rectangle_t*)roi, &thresholds, invert,
					other);
			list_free(&thresholds);
			break;
		}
		case IMAGE_BPP_RGB888: {

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
				return stm32ipl_err_OutOfMemory;
			}

			hist->ABins = xalloc(hist->ABinCount * sizeof(float));
			if (hist->ABins == NULL) {
				xfree(hist->LBins);
				return stm32ipl_err_OutOfMemory;
			}
			hist->BBins = xalloc(hist->BBinCount * sizeof(float));
			if (hist->BBins == NULL) {
				xfree(hist->LBins);
				xfree(hist->ABins);
				return stm32ipl_err_OutOfMemory;
			}

			imlib_get_histogram(hist, (image_t*)img, (rectangle_t*)((roi == NULL) ? &_roi : roi), &thresholds, invert, other);
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
 * The supported formats are Binary, Grayscale, RGB565.
 * @param img	Input image
 * @param stats output LAB statistics if Binary or Grayscale only L statistics are modified
 * @param roi	if not NULL the statistics are contextualized into a region of interest
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GetStatistics(const image_t *img, statistics_t *stats, const rectangle_t *roi)
{
	histogram_t hist;
	stm32ipl_err_t error;

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!stats)
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

		case IMAGE_BPP_RGB888: {
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
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img			Input image
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
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	imlib_gamma_corr(img, gamma_val, contrast, brightness);

	return stm32ipl_err_Ok;
}

/**
 * @brief Calculates an affine transform from three pairs of the corresponding points.
 * 		  The ccorresponding points are src[3] and dst[3].
 * @param src Coordinates of triangle vertices in the source image.
 * @param dst Coordinates of triangle vertices in the destination image.
 * @param affine Contains the 6 float number of the 2×3 matrix of an affine transform.
 * 		  		 The first 3 elements are the first line and the last 3 elements are the second line.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_rotation.c @endlink and @link stm32ipl_demo_face_det.c @endlink
 */
stm32ipl_err_t STM32Ipl_GetAffineTransform(const point_t *src, const point_t *dst, array_t *affine)
{
	if (!src || !dst || !affine)
		return stm32ipl_err_InvalidParameter;

	if (array_length(affine) < 6) {
		array_resize(affine, 6);
	}

	double a[6 * 6], b[6];

	for (int i = 0; i < 3; i++) {
		int j = i * 12;
		int k = i * 12 + 6;
		a[j] = a[k + 3] = src[i].x;
		a[j + 1] = a[k + 4] = src[i].y;
		a[j + 2] = a[k + 5] = 1;
		a[j + 3] = a[j + 4] = a[j + 5] = 0;
		a[k] = a[k + 1] = a[k + 2] = 0;
		b[i * 2] = dst[i].x;
		b[i * 2 + 1] = dst[i].y;
	}

	matd_t *A = matd_create(6, 6);
	matd_t *B = matd_create(6, 1);

	for (int i = 0; i < 3; i++) {
		int j = i * 12;
		int k = i * 12 + 6;
		a[j] = a[k + 3] = src[i].x;
		a[j + 1] = a[k + 4] = src[i].y;
		a[j + 2] = a[k + 5] = 1;
		a[j + 3] = a[j + 4] = a[j + 5] = 0;
		a[k] = a[k + 1] = a[k + 2] = 0;
		b[i * 2] = dst[i].x;
		b[i * 2 + 1] = dst[i].y;
	}

	for (int i = 0; i < 6; i++)
		for (int j = 0; j < 6; j++) {
			MATD_EL(A, i, j)= a[i * 6 + j];
		}

	for (int i = 0; i < 6; i++)
		for (int j = 0; j < 1; j++) {
			MATD_EL(B, i, j)= b[i];
		}

	matd_t *M = matd_solve(A, B);

	float *p = xalloc(6 * sizeof(float)); //TODO check if change in sizeof(float*)

	if (p == NULL) {
		matd_destroy(M);
		return stm32ipl_err_OutOfMemory;
	}

	for (int i = 0; i < 6; i++) {
		p[i] = MATD_EL(M, i, 0);
		array_push_back(affine, (void*)&(p[i]));
	}

	matd_destroy(M);
	matd_destroy(B);
	matd_destroy(A);

	return stm32ipl_err_Ok;
}

/**
 * @brief Applies an affine transformation to an image
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img		Image to apply the warping
 * @param affine	Contains the 6 float number of the 2×3 matrix of an affine transform.
 * 		  		 	The first 3 elements are the first line and the last 3 elements are the second line.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_rotation.c @endlink and @link stm32ipl_demo_face_det.c @endlink
 */
stm32ipl_err_t STM32Ipl_WarpAffine(image_t *img, const array_t *affine)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!affine)
		return stm32ipl_err_InvalidParameter;

	if (array_length((array_t*)affine) < 6)
		return stm32ipl_err_InvalidParameter;

	uint32_t h = img->h;
	uint32_t w = img->w;

	// Create a tmp copy of the image to pull pixels from.
	size_t size = STM32Ipl_ImageDataSize(img);
	void *data = xalloc(size);
	if (data == NULL)
		return stm32ipl_err_OutOfMemory;

	memcpy(data, img->data, size);
	memset(img->data, 0, size);

	float *p = xalloc(9 * sizeof(float));
	if (p == NULL) {
		xfree(data);
		return stm32ipl_err_OutOfMemory;
	}

	for (int i = 0; i < 6; i++) {
		p[i] = ((float*)array_at((array_t*)affine, i))[0];
	}
	p[6] = 0;
	p[7] = 0;
	p[8] = 1;

	matd_t *T3 = matd_create_data(3, 3, p);
	matd_t *T4 = matd_inverse(T3);

	if (T4) {
		float T4_00 = MATD_EL(T4, 0, 0), T4_01 = MATD_EL(T4, 0, 1), T4_02 = MATD_EL(T4, 0, 2);
		float T4_10 = MATD_EL(T4, 1, 0), T4_11 = MATD_EL(T4, 1, 1), T4_12 = MATD_EL(T4, 1, 2);
		float T4_20 = MATD_EL(T4, 2, 0), T4_21 = MATD_EL(T4, 2, 1), T4_22 = MATD_EL(T4, 2, 2);

		if ((fast_fabsf(T4_20) < MATD_EPS) && (fast_fabsf(T4_21) < MATD_EPS)) { // warp affine
			T4_00 /= T4_22;
			T4_01 /= T4_22;
			T4_02 /= T4_22;
			T4_10 /= T4_22;
			T4_11 /= T4_22;
			T4_12 /= T4_22;
			switch (img->bpp) {
				case IMAGE_BPP_BINARY: {
					uint32_t *tmp = (uint32_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							int sourceX = fast_roundf(T4_00 * x + T4_01 * y + T4_02);
							int sourceY = fast_roundf(T4_10 * x + T4_11 * y + T4_12);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								uint32_t *ptr = tmp + (((w + UINT32_T_MASK) >> UINT32_T_SHIFT) * sourceY);
								int pixel = IMAGE_GET_BINARY_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_BINARY_PIXEL_FAST(row_ptr, x, pixel);
							}
						}
					}
					break;
				}
				case IMAGE_BPP_GRAYSCALE: {
					uint8_t *tmp = (uint8_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							int sourceX = fast_roundf(T4_00 * x + T4_01 * y + T4_02);
							int sourceY = fast_roundf(T4_10 * x + T4_11 * y + T4_12);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								uint8_t *ptr = tmp + (w * sourceY);
								int pixel = IMAGE_GET_GRAYSCALE_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr, x, pixel);
							}
						}
					}
					break;
				}
				case IMAGE_BPP_RGB565: {
					uint16_t *tmp = (uint16_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							int sourceX = fast_roundf(T4_00 * x + T4_01 * y + T4_02);
							int sourceY = fast_roundf(T4_10 * x + T4_11 * y + T4_12);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								uint16_t *ptr = tmp + (w * sourceY);
								int pixel = IMAGE_GET_RGB565_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_RGB565_PIXEL_FAST(row_ptr, x, pixel);
							}
						}
					}
					break;
				}

				case IMAGE_BPP_RGB888: {
					rgb888_t *tmp = (rgb888_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						rgb888_t *row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							int sourceX = fast_roundf(T4_00 * x + T4_01 * y + T4_02);
							int sourceY = fast_roundf(T4_10 * x + T4_11 * y + T4_12);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								rgb888_t *ptr = tmp + (w * sourceY);
								rgb888_t pixel = IMAGE_GET_RGB888_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_RGB888_PIXEL_FAST(row_ptr, x, pixel);
							}
						}
					}
					break;
				}

				default: {
					break;
				}
			}
		} else { // warp persepective
			switch (img->bpp) {
				case IMAGE_BPP_BINARY: {
					uint32_t *tmp = (uint32_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							float xxx = T4_00 * x + T4_01 * y + T4_02;
							float yyy = T4_10 * x + T4_11 * y + T4_12;
							float zzz = T4_20 * x + T4_21 * y + T4_22;
							int sourceX = fast_roundf(xxx / zzz);
							int sourceY = fast_roundf(yyy / zzz);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								uint32_t *ptr = tmp + (((w + UINT32_T_MASK) >> UINT32_T_SHIFT) * sourceY);
								int pixel = IMAGE_GET_BINARY_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_BINARY_PIXEL_FAST(row_ptr, x, pixel);
							}
						}
					}
					break;
				}
				case IMAGE_BPP_GRAYSCALE: {
					uint8_t *tmp = (uint8_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							float xxx = T4_00 * x + T4_01 * y + T4_02;
							float yyy = T4_10 * x + T4_11 * y + T4_12;
							float zzz = T4_20 * x + T4_21 * y + T4_22;
							int sourceX = fast_roundf(xxx / zzz);
							int sourceY = fast_roundf(yyy / zzz);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								uint8_t *ptr = tmp + (w * sourceY);
								int pixel = IMAGE_GET_GRAYSCALE_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr, x, pixel);
							}
						}
					}
					break;
				}
				case IMAGE_BPP_RGB565: {
					uint16_t *tmp = (uint16_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							float xxx = T4_00 * x + T4_01 * y + T4_02;
							float yyy = T4_10 * x + T4_11 * y + T4_12;
							float zzz = T4_20 * x + T4_21 * y + T4_22;
							int sourceX = fast_roundf(xxx / zzz);
							int sourceY = fast_roundf(yyy / zzz);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								uint16_t *ptr = tmp + (w * sourceY);
								int pixel = IMAGE_GET_RGB565_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_RGB565_PIXEL_FAST(row_ptr, x, pixel);
							}
						}
					}
					break;
				}

				case IMAGE_BPP_RGB888: {
					rgb888_t *tmp = (rgb888_t*)data;

					for (int y = 0, yy = h; y < yy; y++) {
						rgb888_t *row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
						for (int x = 0, xx = w; x < xx; x++) {
							float xxx = T4_00 * x + T4_01 * y + T4_02;
							float yyy = T4_10 * x + T4_11 * y + T4_12;
							float zzz = T4_20 * x + T4_21 * y + T4_22;
							int sourceX = fast_roundf(xxx / zzz);
							int sourceY = fast_roundf(yyy / zzz);

							if ((0 <= sourceX) && (sourceX < w) && (0 <= sourceY) && (sourceY < h)) {
								rgb888_t *ptr = tmp + (w * sourceY);
								rgb888_t pixel = IMAGE_GET_RGB888_PIXEL_FAST(ptr, sourceX);
								IMAGE_PUT_RGB888_PIXEL_FAST(row_ptr, x, pixel);
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
	}

	matd_destroy(T4);
	matd_destroy(T3);
	xfree(p);
	xfree(data);

	return stm32ipl_err_Ok;
}

/**
 * @brief Applies an affine transformation to a points
 * @param points	points to apply the warping
 * @param affine	Contains the 6 float number of the 2×3 matrix of an affine transform.
 * 		  		 	The first 3 elements are the first line and the last 3 elements are the second line.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_face_det.c @endlink
 */
stm32ipl_err_t STM32Ipl_WarpAffinePoints(array_t *points, const array_t *affine)
{
	if (!points || !affine)
		return stm32ipl_err_InvalidParameter;

	if (array_length((array_t*)affine) < 6)
		return stm32ipl_err_InvalidParameter;

	float *p = xalloc(9 * sizeof(float));
	if (p == NULL)
		return stm32ipl_err_OutOfMemory;

	for (int i = 0; i < 6; i++) {
		p[i] = ((float*)array_at((array_t*)affine, i))[0];
	}
	p[6] = 0;
	p[7] = 0;
	p[8] = 1;

	matd_t *T4 = matd_create_data(3, 3, p);

	if (T4) {
		float T4_00 = MATD_EL(T4, 0, 0), T4_01 = MATD_EL(T4, 0, 1), T4_02 = MATD_EL(T4, 0, 2);
		float T4_10 = MATD_EL(T4, 1, 0), T4_11 = MATD_EL(T4, 1, 1), T4_12 = MATD_EL(T4, 1, 2);
		float T4_20 = MATD_EL(T4, 2, 0), T4_21 = MATD_EL(T4, 2, 1), T4_22 = MATD_EL(T4, 2, 2);

		if ((fast_fabsf(T4_20) < MATD_EPS) && (fast_fabsf(T4_21) < MATD_EPS)) { // warp affine
			T4_00 /= T4_22;
			T4_01 /= T4_22;
			T4_02 /= T4_22;
			T4_10 /= T4_22;
			T4_11 /= T4_22;
			T4_12 /= T4_22;

			for (uint32_t idx = 0; idx < array_length(points); idx++) {
				point_t *point = array_at(points, idx);
				int sourceX = fast_roundf(T4_00 * point->x + T4_01 * point->y + T4_02);
				int sourceY = fast_roundf(T4_10 * point->x + T4_11 * point->y + T4_12);
				point->x = sourceX;
				point->y = sourceY;
			}

		} else { // warp persepective

			for (uint32_t idx = 0; idx < array_length(points); idx++) {
				point_t *point = array_at(points, idx);
				float xxx = T4_00 * point->x + T4_01 * point->y + T4_02;
				float yyy = T4_10 * point->x + T4_11 * point->y + T4_12;
				float zzz = T4_20 * point->x + T4_21 * point->y + T4_22;
				int sourceX = fast_roundf(xxx / zzz);
				int sourceY = fast_roundf(yyy / zzz);

				point->x = sourceX;
				point->y = sourceY;
			}
		}
	}

	matd_destroy(T4);
	xfree(p);

	return stm32ipl_err_Ok;
}

/**
 * @brief Tries to find the first location in the image where template matches
 * 		  using Normalized Cross Correlation.
 * The supported format is Grayscale.
 * @param img			  Input image
 * @param template        Input image to find in img
 * @param thresh		  is floating point number (0.0-1.0) where
 * 						  a higher threshold prevents false positives while lowering the
 * 						  detection rate while a lower threshold does the opposite
 * @param roi			  region of interest, NULL extends the search to the entire image
 * @param step			  is the number of pixels to skip past while looking for the template.
 * 						  Skipping pixels considerably speeds the algorithm up.
 * 						  This only affects the algorithm in SERACH_EX mode.
 * @param search		  can be either SEARCH_DS or SEARCH_EX.
 * 						  SEARCH_DS searches for the template using as faster algorithm than SEARCH_EX
 * 						  but may not find the template if it’s near the edges of the image.
 * 						  SEARCH_EX does an exhaustive search for the image but can be much slower than SEARCH_DS
 * @param findedTemplate  location of the found template. If not found x,y,w,h set to 0.
 * @param corrFindedTemplate correlation between template and findedTemplate.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_template_matching.c @endlink
 */

stm32ipl_err_t STM32Ipl_FindTemplate(const image_t *img, const image_t *template, float thresh, rectangle_t *roi,
		int step, int search, rectangle_t *findedTemplate, float *corrFindedTemplate)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(template)

	if (img->bpp != IMAGE_BPP_GRAYSCALE)
		return stm32ipl_err_UnsupportedFormat;

	STM32IPL_CHECK_SAME_FORMAT(img, template)

	if (!findedTemplate)
		return stm32ipl_err_InvalidParameter;

	rectangle_t _roi = { 0, 0, 0, 0 };
	if (roi == NULL) {
		_roi.w = img->w;
		_roi.h = img->h;
	} else {
		_roi.x = roi->x;
		_roi.y = roi->y;
		_roi.w = roi->w;
		_roi.h = roi->h;
	}

	// Make sure ROI is bigger than or equal to template size
	if ((_roi.w < template->w || _roi.h < template->h))
		return stm32ipl_err_InvalidParameter;

	// Make sure ROI is smaller than or equal to image size
	if (((_roi.x + _roi.w) > img->w || (_roi.y + _roi.h) > img->h))
		return stm32ipl_err_InvalidParameter;

	// Find template
	float corr;

	if (search == SEARCH_DS) {
		corr = imlib_template_match_ds((image_t*)img, (image_t*)template, findedTemplate);
	} else {
		corr = imlib_template_match_ex((image_t*)img, (image_t*)template, &_roi, step, findedTemplate);
	}

	if (corr < thresh) {
		findedTemplate->x = 0;
		findedTemplate->y = 0;
		findedTemplate->w = 0;
		findedTemplate->h = 0;
	}

	*corrFindedTemplate = corr;

	return stm32ipl_err_Ok;
}

/**
 * @brief Count non zero Pixel in a image
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
 * @param result	number of non zero pixel
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_CountNonZero(const image_t *img, uint32_t *result)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!result)
		return stm32ipl_err_InvalidParameter;

	*result = 0;

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					if (IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, x) > 0) {
						*result += 1;
					}
				}
			}

		}
			break;

		case IMAGE_BPP_GRAYSCALE: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					if (IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x) > 0) {
						*result += 1;
					}
				}
			}

		}
			break;

		case IMAGE_BPP_RGB565: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					if (COLOR_RGB565_TO_GRAYSCALE(IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x)) > 0) {
						*result += 1;
					}
				}
			}

		}
			break;

		case IMAGE_BPP_RGB888: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				rgb888_t *row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					if (COLOR_RGB888_TO_GRAYSCALE(IMAGE_GET_RGB888_PIXEL_FAST(row_ptr, x)) > 0) {
						*result += 1;
					}
				}
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
 * @brief Find non zero Pixel in a Image
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img		Input image
 * @param result	list of point_t coordinates of non zero pixel
 * @param roi		region of interest to find non zero if NULL used the full image size
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_pixel_counting.c @endlink
 */
stm32ipl_err_t STM32Ipl_FindNonZero(const image_t *img, list_t *result, rectangle_t *roi)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!result)
		return stm32ipl_err_InvalidParameter;

	point_t p;
	uint32_t i = 0;
	uint32_t start_x = 0;
	uint32_t stop_x = img->w;
	uint32_t start_y = 0;
	uint32_t stop_y = img->h;

	if (roi != NULL) {
		start_x = roi->x;
		start_y = roi->y;
		stop_x = roi->x + roi->w;
		stop_y = roi->y + roi->h;
	}

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {

			for (int y = start_y, yy = stop_y; y < yy; y++) {
				uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
				for (int x = start_x, xx = stop_x; x < xx; x++) {
					if (IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, x) > 0) {
						p.x = x;
						p.y = y;
						list_insert(result, &p, i);

						if (result->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;
					}
				}
			}

		}
			break;

		case IMAGE_BPP_GRAYSCALE: {

			for (int y = start_y, yy = stop_y; y < yy; y++) {
				uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
				for (int x = start_x, xx = stop_x; x < xx; x++) {
					if (IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x) > 0) {

						p.x = x;
						p.y = y;

						list_insert(result, &p, i);

						if (result->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;
					}
				}
			}

		}
			break;

		case IMAGE_BPP_RGB565: {

			for (int y = start_y, yy = stop_y; y < yy; y++) {
				uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
				for (int x = start_x, xx = stop_x; x < xx; x++) {
					if (COLOR_RGB565_TO_GRAYSCALE(IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x)) > 0) {

						p.x = x;
						p.y = y;
						list_insert(result, &p, i);

						if (result->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;
					}
				}
			}

		}
			break;

		case IMAGE_BPP_RGB888: {

			for (int y = start_y, yy = stop_y; y < yy; y++) {
				rgb888_t *row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
				for (int x = start_x, xx = stop_x; x < xx; x++) {
					if (COLOR_RGB888_TO_GRAYSCALE(IMAGE_GET_RGB888_PIXEL_FAST(row_ptr, x)) > 0) {

						p.x = x;
						p.y = y;
						list_insert(result, &p, i);

						if (result->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;
					}
				}
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
 * @brief Find minimum and maximum point location in a Image. The minimum and maximum value for RGB is the Y value
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img Input image
 * @param resultMin	list of point_t minimum values
 * @param resultMax	list of point_t maximum values
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_MinMaxLoc(const image_t *img, list_t *resultMin, list_t *resultMax)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!resultMin || !resultMax)
		return stm32ipl_err_InvalidParameter;

	uint32_t max = 0;
	uint32_t min = 0xFFFFFFFF;
	uint32_t i = 0;
	uint32_t j = 0;

	bool minVal = false;
	bool maxVal = false;

	point_t p;
	p.x = 0;
	p.y = 0;
	list_push_front(resultMin, &p);
	list_push_front(resultMax, &p);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					uint32_t value = IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, x);
					if (value < min) {
						min = value;
						list_clear(resultMin);
						i = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMin, &p, i);

						if (resultMin->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;

						minVal = false;

					} else
						if (value == min) {

							p.x = x;
							p.y = y;
							list_insert(resultMin, &p, i);

							if (resultMin->size == i)
								minVal = true; //to continue search new min before return stm32ipl_err_OutOfMemory;
							else
								i += 1;
						}

					if (value > max) {
						max = value;
						list_clear(resultMax);
						j = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMax, &p, j);

						if (resultMax->size == j)
							return stm32ipl_err_OutOfMemory;
						else
							j += 1;

						maxVal = false;

					} else
						if (value == max) {

							p.x = x;
							p.y = y;
							list_insert(resultMax, &p, j);

							if (resultMax->size == j)
								maxVal = true; //to continue search new max before return stm32ipl_err_OutOfMemory;
							else
								j += 1;

						}
				}
			}

		}
			break;

		case IMAGE_BPP_GRAYSCALE: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					uint8_t value = IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x);

					if (value < min) {
						min = value;
						list_clear(resultMin);
						i = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMin, &p, i);

						if (resultMin->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;

						minVal = false;

					} else
						if (value == min) {

							p.x = x;
							p.y = y;
							list_insert(resultMin, &p, i);

							if (resultMin->size == i)
								minVal = true; //to continue search new min before return stm32ipl_err_OutOfMemory;
							else
								i += 1;

						}

					if (value > max) {
						max = value;
						list_clear(resultMax);
						j = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMax, &p, j);

						if (resultMax->size == j)
							return stm32ipl_err_OutOfMemory;
						else
							j += 1;

						maxVal = false;

					} else
						if (value == max) {
							p.x = x;
							p.y = y;
							list_insert(resultMax, &p, j);

							if (resultMax->size == j)
								maxVal = true; //to continue search new max before return stm32ipl_err_OutOfMemory;
							else
								j += 1;

						}
				}
			}
		}
			break;

		case IMAGE_BPP_RGB565: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					uint8_t value = COLOR_RGB565_TO_GRAYSCALE(IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x));
					if (value < min) {
						min = value;
						list_clear(resultMin);
						i = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMin, &p, i);

						if (resultMin->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;

						minVal = false;

					} else
						if (value == min) {

							p.x = x;
							p.y = y;
							list_insert(resultMin, &p, i);

							if (resultMin->size == i)
								minVal = true; //to continue search new min before return stm32ipl_err_OutOfMemory;
							else
								i += 1;

						}

					if (value > max) {
						max = value;
						list_clear(resultMax);
						j = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMax, &p, j);

						if (resultMax->size == j)
							return stm32ipl_err_OutOfMemory;
						else
							j += 1;

						maxVal = false;

					} else
						if (value == max) {
							p.x = x;
							p.y = y;
							list_insert(resultMax, &p, j);

							if (resultMax->size == j)
								maxVal = true; //to continue search new max before return stm32ipl_err_OutOfMemory;
							else
								j += 1;

						}
				}
			}
		}
			break;

		case IMAGE_BPP_RGB888: {

			for (int y = 0, yy = img->h; y < yy; y++) {
				rgb888_t *row_ptr = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, y);
				for (int x = 0, xx = img->w; x < xx; x++) {
					uint8_t value = COLOR_RGB888_TO_GRAYSCALE(IMAGE_GET_RGB888_PIXEL_FAST(row_ptr, x));
					if (value < min) {
						min = value;
						list_clear(resultMin);
						i = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMin, &p, i);

						if (resultMin->size == i)
							return stm32ipl_err_OutOfMemory;
						else
							i += 1;

						minVal = false;

					} else
						if (value == min) {

							p.x = x;
							p.y = y;
							list_insert(resultMin, &p, i);

							if (resultMin->size == i)
								minVal = true; //to continue search new min before return stm32ipl_err_OutOfMemory;
							else
								i += 1;

						}

					if (value > max) {
						max = value;
						list_clear(resultMax);
						j = 0;
						p.x = x;
						p.y = y;
						list_insert(resultMax, &p, j);

						if (resultMax->size == j)
							return stm32ipl_err_OutOfMemory;
						else
							j += 1;

						maxVal = false;

					} else
						if (value == max) {
							p.x = x;
							p.y = y;
							list_insert(resultMax, &p, j);

							if (resultMax->size == j)
								maxVal = true; //to continue search new max before return stm32ipl_err_OutOfMemory;
							else
								j += 1;

						}
				}
			}
		}
			break;
		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	if (maxVal == true || minVal == true)
		return stm32ipl_err_OutOfMemory; // to advertise if the maxVal or minVal is not contains all values

	return stm32ipl_err_Ok;
}

// TODO implement STM32Ipl_FindBlobsThreshold_cb
static bool STM32Ipl_FindBlobsThreshold_cb(void *fun_obj, find_blobs_list_lnk_data_t *blob)
{
	/* Implement the fun_obj and return the value true or false*/
	return true;
}

// TODO implement STM32Ipl_FindBlobsMerge_cb
static bool STM32Ipl_FindBlobsMerge_cb(void *fun_obj, find_blobs_list_lnk_data_t *blob0,
		find_blobs_list_lnk_data_t *blob1)
{
	/* Implement the fun_obj and return the value true or false*/
	return true;
}

/**
 * @brief Finds all blobs (connected pixel regions that pass a threshold test) in the image
 * 		  and returns a list of blob objects (find_blobs_list_lnk_data_t) which describe each blob.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img				Input image
 * @param out				list of blob objects, find_blobs_list_lnk_data_t
 * @param thresholds		defining the ranges of color you want to track in LAB space (color_thresholds_list_lnk_data_t)
 * @param roi				is the region-of-interest rectangle, if null use all image
 * @param x_stride			is the number of x pixels to skip when searching for a blob.
 * 							Once a blob is found the line fill algorithm will be pixel accurate.
 * 							Increase x_stride to speed up finding blobs if blobs are known to be large.
 * @param y_stride			is the number of y pixels to skip when searching for a blob.
 * 							Once a blob is found the line fill algorithm will be pixel accurate.
 * 							Increase y_stride to speed up finding blobs if blobs are known to be large.
 * @param area_threshold	filter out the blob with bounding box area lesser than area_threshold.
 * @param pixels_threshold  filter out the blob with the pixel are lesser than pixels_threshold.
 * @param merge				if true merges all not filtered out blobs whos bounding rectangles intersect each other.
 * @param margin			can be used to increase or decrease the size of the bounding rectangles for blobs during the intersection test.
 * 							For example, with a margin of 1 blobs whos bounding rectangles are 1 pixel away from each other will be merged.
 * @param invert			inverts the thresholding operation such that instead of matching pixels inside of some known color bounds pixels
 * 							are matched that are outside of the known color bounds.
 * @return	stm32ipl_err_Ok on succes
 */
stm32ipl_err_t STM32Ipl_FindBlobs(const image_t *img, list_t *out, const list_t *thresholds, const rectangle_t *roi,
		uint8_t x_stride, uint8_t y_stride, uint16_t area_threshold, uint16_t pixels_threshold, bool merge,
		uint8_t margin, bool invert)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!thresholds || !out || (list_size((list_t*)thresholds) == 0))
		return stm32ipl_err_InvalidParameter;

	rectangle_t _roi = { 0, 0, 0, 0 };

	if (roi == NULL) {
		_roi.w = img->w;
		_roi.h = img->h;
	}

	if (x_stride == 0 || y_stride == 0)
		return stm32ipl_err_InvalidParameter;

	unsigned int x_hist_bins_max = 0;
	unsigned int y_hist_bins_max = 0;

	imlib_find_blobs(out, (image_t*)img, (roi == NULL) ? &_roi : (rectangle_t*)roi, x_stride, y_stride,
			(list_t*)thresholds, invert, area_threshold, pixels_threshold, merge, margin,
			STM32Ipl_FindBlobsThreshold_cb, NULL, STM32Ipl_FindBlobsMerge_cb, NULL, x_hist_bins_max, y_hist_bins_max);

	return stm32ipl_err_Ok;
}

/**
 * @brief Converts the a rotatedRectangle structure to the corresponding four points clockwise
 * @param r			Rotated rectangle
 * @param points	points[4]
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_BoxPoints(const rotatedRect_t *r, point_t *points)
{
	if (!r || !points)
		return stm32ipl_err_InvalidParameter;

	int x;
	int y;
	uint8_t i = 0;

	x = r->center.x - (r->w / 2);
	y = r->center.y - (r->h / 2);
	point_rotate(x, y, r->rotation, r->center.x, r->center.y, &(points[i].x), &(points[i].y));
	i++;
	x = r->center.x + (r->w / 2);
	y = r->center.y - (r->h / 2);
	point_rotate(x, y, r->rotation, r->center.x, r->center.y, &(points[i].x), &(points[i].y));
	i++;
	x = r->center.x + (r->w / 2);
	y = r->center.y + (r->h / 2);
	point_rotate(x, y, r->rotation, r->center.x, r->center.y, &(points[i].x), &(points[i].y));
	i++;
	x = r->center.x - (r->w / 2);
	y = r->center.y + (r->h / 2);
	point_rotate(x, y, r->rotation, r->center.x, r->center.y, &(points[i].x), &(points[i].y));

	return stm32ipl_err_Ok;
}

/**
 * @brief Draws point over image at location x, y width color
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img		Input image
 * @param x			x
 * @param y			y
 * @param color		is an ARGB8888
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_DrawPixel(image_t *img, uint16_t x, uint16_t y, uint32_t color)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	// FIXME: aggiungi qui la chiamata alla funzione di conversione di color

	uint8_t b = (color & 0xFF);
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;

	uint16_t color_omv;

	color_omv = COLOR_R8_G8_B8_TO_RGB565(r, g, b);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			color_omv = COLOR_RGB565_TO_BINARY(color_omv);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			color_omv = COLOR_RGB565_TO_GRAYSCALE(color_omv);
			break;
		}

		case IMAGE_BPP_RGB565: {
			break;
		}

		case IMAGE_BPP_RGB888: {
			break;
		}

		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	imlib_set_pixel(img, x, y, color_omv);

	return stm32ipl_err_Ok;
}

/**
 * @brief Draws a cross over image at location x, y
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img		Input image
 * @param x			x center of the cross
 * @param y			y center of the cross
 * @param size		controls how long the lines of the cross extend
 * @param color		is an ARGB8888
 * @param thickness	controls how thick the edges are in pixels
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_DrawCross(image_t *img, uint16_t x, uint16_t y, uint8_t size, uint32_t color, uint8_t thickness)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	uint8_t s = size / 2;
	uint8_t b = (color & 0xFF);
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;

	// FIXME: aggiungi qui la chiamata alla funzione di conversione di color

	uint16_t color_omv;

	color_omv = COLOR_R8_G8_B8_TO_RGB565(r, g, b);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			color_omv = COLOR_RGB565_TO_BINARY(color_omv);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			color_omv = COLOR_RGB565_TO_GRAYSCALE(color_omv);
			break;
		}

		case IMAGE_BPP_RGB565: {
			break;
		}

		case IMAGE_BPP_RGB888: {
			break;
		}

		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	imlib_draw_line(img, x - s, y, x + s, y, color, thickness);
	imlib_draw_line(img, x, y - s, x, y + s, color, thickness);

	return stm32ipl_err_Ok;
}

/**
 * @brief Draws a line over img from p0 to p1 on the image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img		Input image
 * @param p0		line draw start point
 * @param p1		line draw stop pont
 * @param color		is an ARGB8888
 * @param thickness	controls how thick the edges are in pixels
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_DrawLine(image_t *img, point_t *p0, point_t *p1, uint32_t color, uint8_t thickness)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!p0 || !p1)
		return stm32ipl_err_InvalidParameter;

	// FIXME: aggiungi qui la chiamata alla funzione di conversione di color

	uint8_t b = (color & 0xFF);
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;

	uint16_t color_omv;

	color_omv = COLOR_R8_G8_B8_TO_RGB565(r, g, b);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			color_omv = COLOR_RGB565_TO_BINARY(color_omv);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			color_omv = COLOR_RGB565_TO_GRAYSCALE(color_omv);
			break;
		}

		case IMAGE_BPP_RGB565: {
			break;
		}

		case IMAGE_BPP_RGB888: {
			break;
		}

		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	imlib_draw_line(img, p0->x, p0->y, p1->x, p1->y, color_omv, thickness);

	return stm32ipl_err_Ok;
}

/**
 * @brief Draws a multiple line over img using point[nPoints]
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img		Input image
 * @param point		points to connect, point[nPoints]
 * @param nPoints	number of points vertex of polygon
 * @param color		is an ARGB8888
 * @param thickness	controls how thick the edges are in pixels
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_DrawPolygon(image_t *img, point_t *point, uint32_t nPoints, uint32_t color, uint8_t thickness)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!point)
		return stm32ipl_err_InvalidParameter;

	// FIXME: aggiungi qui la chiamata alla funzione di conversione di color

	uint8_t b = (color & 0xFF);
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;

	uint16_t color_omv;

	color_omv = COLOR_R8_G8_B8_TO_RGB565(r, g, b);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			color_omv = COLOR_RGB565_TO_BINARY(color_omv);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			color_omv = COLOR_RGB565_TO_GRAYSCALE(color_omv);
			break;
		}

		case IMAGE_BPP_RGB565: {
			break;
		}

		case IMAGE_BPP_RGB888: {
			break;
		}

		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	imlib_draw_line(img, point[0].x, point[0].y, point[nPoints - 1].x, point[nPoints - 1].y, color_omv, thickness);

	for (uint8_t j = 0; j < nPoints - 1; j++) {
		imlib_draw_line(img, point[j].x, point[j].y, point[j + 1].x, point[j + 1].y, color_omv, thickness);
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Draws a rectangle on the image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	Input image
 * @param x		The x coordinate of the top-left corner of the image.
 * @param y		The y coordinate of the top-left corner of the image.
 * @param w		The width of rectangle
 * @param h		The height of rectangle
 * @param color	is an ARGB8888 color
 * @param thickness controls how thick the edges are in pixels
 * @param fill		control if the rectangle is filled or not.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_DrawRectangle(image_t *img, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color,
		uint8_t thickness, bool fill)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (w < 2 || h < 2)
		return stm32ipl_err_InvalidParameter;

// FIXME: aggiungi qui la chiamata alla funzione di conversione di color


	uint8_t b = (color & 0xFF);
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;

	uint16_t color_omv;

	color_omv = COLOR_R8_G8_B8_TO_RGB565(r, g, b);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			color_omv = COLOR_RGB565_TO_BINARY(color_omv);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			color_omv = COLOR_RGB565_TO_GRAYSCALE(color_omv);
			break;
		}

		case IMAGE_BPP_RGB565: {
			break;
		}

		case IMAGE_BPP_RGB888: {
			break;
		}

		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	imlib_draw_rectangle(img, x, y, w, h, color_omv, thickness, fill);

	return stm32ipl_err_Ok;
}

/**
 * @brief Draws a circle over image. You may either pass cx, cy, radius.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	Input image
 * @param cx	x center of circle
 * @param cy	y center of circle
 * @param radius	radius of the circle
 * @param color	is an ARGB8888 color
 * @param thickness controls how thick the edges are in pixels
 * @param fill		control if the circle is filled or not.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_DrawCircle(image_t *img, uint16_t cx, uint16_t cy, uint16_t radius, uint32_t color,
		uint8_t thickness, bool fill)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	// FIXME: aggiungi qui la chiamata alla funzione di conversione di color

	uint8_t b = (color & 0xFF);
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;

	uint16_t color_omv;

	color_omv = COLOR_R8_G8_B8_TO_RGB565(r, g, b);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			color_omv = COLOR_RGB565_TO_BINARY(color_omv);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			color_omv = COLOR_RGB565_TO_GRAYSCALE(color_omv);
			break;
		}

		case IMAGE_BPP_RGB565: {
			break;
		}

		case IMAGE_BPP_RGB888: {
			break;
		}

		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	imlib_draw_circle(img, cx, cy, radius, color_omv, thickness, fill);

	return stm32ipl_err_Ok;
}

/**
 * @brief Draws an ellipse on the image.
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img	Input image
 * @param cx	x center of circle
 * @param cy	y center of circle
 * @param rx	max axis
 * @param ry	min axis
 * @param rotation	ellipse rotation
 * @param color	is an ARGB8888 color
 * @param thickness controls how thick the edges are in pixels
 * @param fill		control if the circle is filled or not.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_DrawEllipse(image_t *img, uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, uint16_t rotation,
		uint32_t color, uint8_t thickness, bool fill)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	// FIXME: aggiungi qui la chiamata alla funzione di conversione di color

	uint8_t b = (color & 0xFF);
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;

	uint16_t color_omv;

	color_omv = COLOR_R8_G8_B8_TO_RGB565(r, g, b);

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			color_omv = COLOR_RGB565_TO_BINARY(color_omv);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			color_omv = COLOR_RGB565_TO_GRAYSCALE(color_omv);
			break;
		}

		case IMAGE_BPP_RGB565: {
			break;
		}

		case IMAGE_BPP_RGB888: {
			break;
		}

		default: {
			return stm32ipl_err_InvalidParameter;
		}
	}

	imlib_draw_ellipse(img, cx, cy, rx, ry, rotation, color_omv, thickness, fill);

	return stm32ipl_err_Ok;
}

/**
 * @brief Computes a linear regression on all the thresholded pixels in the image.
 * 		  The linear regression is computed using least-squares normally which is fast but cannot handle any outliers.
 * 		  If robust is True then the Theil–Sen linear regression is used instead which computes the median of all slopes between all thresholded pixels in the image
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param out	output and contains find_lines_list_lnk_data with the line points, magnitude.
 * @param img	Input image
 * @param roi	region of interest, if NULL, it is equal to the image rectangle
 * @param x_stride	is the number of x pixels to skip over when evaluating the image.
 * @param y_stride	is the number of y pixels to skip over when evaluating the image.
 * @param thresholds	List of color_thresholds_list_lnk_data_t. For grayscale images use LMin, LMax
 * 						and for RGB565 use LMin, LMax, AMin, AMax, BMin, BMax (Threshold in LAB color space)
 * @param invert		inverts the thresholding operation such that instead of matching pixels inside of some known color bounds pixels are matched that are outside of the known color bounds.
 * @param area_threshold	If the regression’s bounding box area is less than area_threshold then out values are set to zero.
 * @param pixels_threshold	If the regression’s pixel count is less than pixels_threshold then then out values are set to zero.
 * @param robust			is True then the Theil–Sen linear regression is used instead which computes the median of all slopes between all thresholded pixels in the image.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_regression.c @endlink
 */
stm32ipl_err_t STM32Ipl_GetRegressionImage(find_lines_list_lnk_data_t *out, const image_t *img, const rectangle_t *roi,
		uint8_t x_stride, uint8_t y_stride, const list_t *thresholds, bool invert, unsigned int area_threshold,
		unsigned int pixels_threshold, bool robust)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)

	if (!out || !thresholds || list_size((list_t*)thresholds) == 0)
		return stm32ipl_err_InvalidParameter;

	bool res = false;
	rectangle_t _roi = { 0, 0, 0, 0 };
	if (roi == NULL) {
		_roi.w = img->w;
		_roi.h = img->h;
	}

	res = imlib_get_regression(out, (image_t*)img, ((roi == NULL) ? &_roi : (rectangle_t*)roi), x_stride, y_stride,
			(list_t*)thresholds, invert, area_threshold, pixels_threshold, robust);

	if (res == true)
		return stm32ipl_err_Ok;

	return stm32ipl_err_OpNotCompleted;
}

/**
 * @brief singular value decomposition (SVD) is a factorisation of a real or complex matrix
 * 		  that generalises the eigen decomposition of a square normal matrix to any m x n matrix via an extension of the polar decomposition.
 * @param pointx	x values of points
 * @param pointy	y values of points
 * @param nPoints	n points (pointx and pointy have the same lenght)
 * @param u	is MxM (and is an orthonormal basis). u[0..M-1] is the first line, u[M..2M-1] is the second line etc..
 * @param s is MxN (and is diagonal up to machine precision)
 * @param v is NxN (and is an orthonormal basis).
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Svd(const float *pointx, const float *pointy, uint16_t nPoints, float *u, float *s, float *v)
{
	if (!pointx || !pointy || !u || !s || !v || nPoints < 2)
		return stm32ipl_err_InvalidParameter;

	matd_t *A = matd_create(2, nPoints);

	for (int i = 0; i < nPoints; i++) {
		MATD_EL(A, 0, i)= pointx[i];
		MATD_EL(A, 1, i) = pointy[i];
	}

	matd_svd_t svd = matd_svd(A);

	/* U is MxM (and is an orthonormal basis) */
	/* S is MxN (and is diagonal up to machine precision) */
	/* V is NxN (and is an orthonormal basis). */
	/* N = 300 */
	/* M = 2 */

	for (int i = 0; i < svd.U->ncols * svd.U->nrows; i++)
		u[i] = svd.U->data[i];

	for (int i = 0; i < svd.S->ncols * svd.S->nrows; i++)
		s[i] = svd.S->data[i];

	for (int i = 0; i < svd.V->ncols * svd.V->nrows; i++)
		v[i] = svd.V->data[i];

	/*
	 memcpy(u, &svd.U->data[0], svd.U->ncols * svd.U->nrows * sizeof(float));
	 memcpy(s, &svd.S->data[0], svd.S->ncols * svd.S->nrows * sizeof(float));
	 memcpy(v, &svd.V->data[0], svd.V->ncols * svd.V->nrows * sizeof(float));
	 */

	matd_destroy(svd.U);
	matd_destroy(svd.S);
	matd_destroy(svd.V);

	return stm32ipl_err_Ok;
}

/**
 * @brief Returns a description of a circle using it's centre and it's ray
 * @param points	four points. If extracted using point_min_area_rectangle, the circle that encloses the min area rectagle.
 * @param nPoints	all four points
 * @param c			center of circle
 * @param dmax		ray of circle
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_EnclosingCircle(const point_t *points, uint16_t nPoints, point_t *c, uint16_t *dmax)
{

	if (!points || !c || !dmax)
		return stm32ipl_err_InvalidParameter;

	if (nPoints != 4)
		return stm32ipl_err_InvalidParameter;

	int16_t x0, y0, x1, y1, x2, y2, x3, y3;

	x0 = points[0].x;
	y0 = points[0].y;
	x1 = points[1].x;
	y1 = points[1].y;
	x2 = points[2].x;
	y2 = points[2].y;
	x3 = points[3].x;
	y3 = points[3].y;

	uint16_t cx = (x0 + x1 + x2 + x3) / 4;
	uint16_t cy = (y0 + y1 + y2 + y3) / 4;

	float d0 = fast_sqrtf(((x0 - cx) * (x0 - cx)) + ((y0 - cy) * (y0 - cy)));
	float d1 = fast_sqrtf(((x1 - cx) * (x1 - cx)) + ((y1 - cy) * (y1 - cy)));
	float d2 = fast_sqrtf(((x2 - cx) * (x2 - cx)) + ((y2 - cy) * (y2 - cy)));
	float d3 = fast_sqrtf(((x3 - cx) * (x3 - cx)) + ((y3 - cy) * (y3 - cy)));
	float d = IM_MAX(d0, IM_MAX(d1, IM_MAX(d2, d3)));

	c->x = cx;
	c->y = cy;
	*dmax = fast_roundf(d);

	return stm32ipl_err_Ok;

}

/**
 * @brief Returns a description of a ellipse using it's centre, it's max axis, it's min axis and it's rotation
 * @param points	four points. If extracted using point_min_area_rectangle, the ellipse that encloses the min area rectagle.
 * @param nPoints always four points
 * @param c			ellipse centre
 * @param max		max axis
 * @param min		min axis
 * @param rotation	rotation
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_EnclosingEllipse(const point_t *points, uint16_t nPoints, point_t *c, float *max, float *min,
		float *rotation)
{

	if (!points || !c || !max || !min || !rotation)
		return stm32ipl_err_InvalidParameter;

	if (nPoints != 4)
		return stm32ipl_err_InvalidParameter;

	int16_t x0, y0, x1, y1, x2, y2, x3, y3;

	x0 = points[0].x;
	y0 = points[0].y;
	x1 = points[1].x;
	y1 = points[1].y;
	x2 = points[2].x;
	y2 = points[2].y;
	x3 = points[3].x;
	y3 = points[3].y;

	int16_t m0x = (x0 + x1) / 2;
	int16_t m0y = (y0 + y1) / 2;
	int16_t m1x = (x1 + x2) / 2;
	int16_t m1y = (y1 + y2) / 2;
	int16_t m2x = (x2 + x3) / 2;
	int16_t m2y = (y2 + y3) / 2;
	int16_t m3x = (x3 + x0) / 2;
	int16_t m3y = (y3 + y0) / 2;

	uint16_t cx = (x0 + x1 + x2 + x3) / 4;
	uint16_t cy = (y0 + y1 + y2 + y3) / 4;

	float d0 = fast_sqrtf(((m0x - cx) * (m0x - cx)) + ((m0y - cy) * (m0y - cy)));
	float d1 = fast_sqrtf(((m1x - cx) * (m1x - cx)) + ((m1y - cy) * (m1y - cy)));
	float d2 = fast_sqrtf(((m2x - cx) * (m2x - cx)) + ((m2y - cy) * (m2y - cy)));
	float d3 = fast_sqrtf(((m3x - cx) * (m3x - cx)) + ((m3y - cy) * (m3y - cy)));
	float a = IM_MIN(d0, d2);
	float b = IM_MIN(d1, d3);

	float l0 = fast_sqrtf(((m0x - m2x) * (m0x - m2x)) + ((m0y - m2y) * (m0y - m2y)));
	float l1 = fast_sqrtf(((m1x - m3x) * (m1x - m3x)) + ((m1y - m3y) * (m1y - m3y)));

	float r;

	if (l0 >= l1) {
		r = IM_RAD2DEG(fast_atan2f(m0y - m2y, m0x - m2x));
	} else {
		r = IM_RAD2DEG(fast_atan2f(m1y - m3y, m1x - m3x) + M_PI_2);
	}

	c->x = cx;
	c->y = cy;
	*min = a;
	*max = b;
	*rotation = r;

	return stm32ipl_err_Ok;
}

// FIXME: to be substituted with the new one.
/**
 * @brief fit points [x,y] using enclosed_ellipse and calculate the rotation using line regression
 * @param x			x values of points
 * @param y			y values of points
 * @param nPoints	define the number of x and y values
 * @param fit		2 * M [x and y] values that describe the fitting ellipse
 * @param M			number of points to describe the fit ellipse
 * @param ellipse	ellipse (with the rotation to describe an ellipse)
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_regression.c @endlink
 */
stm32ipl_err_t STM32Ipl_FitEllipse(const uint16_t *x, const uint16_t *y, uint16_t nPoints, float *fit, uint8_t M,
		ellipse_t *ellipse)
{
	//float xx[nPoints];
	float *xx;
	//float yy[nPoints];
	float *yy;
	float mean_x = 0.0f;
	float mean_y = 0.0f;
	float sum_x = 0.0f;
	float sum_y = 0.0f;
	float U[2 * 2];
	//float S[nPoints * 2];
	float *S;
	//float V[nPoints * nPoints];
	float *V;
	//float array2[M];
	float *array2;
	//float circle[M * 2];
	float *circle;
	float transform[4];
	float start = 0.0f;
	float stop;
	float step;

	point_t center;
	point_t point;
	float dmax;
	float dmin;
	float rot;
	find_lines_list_lnk_data_t line_points;
	//point_t points[M];
	point_t *points;
	point_t min_corners[4];

	int dh;
	int dl;
	float ipo;
	float angle;

	if (!x || !y || !fit || !ellipse)
		return stm32ipl_err_InvalidParameter;

	xx = xalloc(nPoints * sizeof(float));
	if (!xx)
		return stm32ipl_err_OutOfMemory;

	yy = xalloc(nPoints * sizeof(float));
	if (!yy)
		return stm32ipl_err_OutOfMemory;

	S = xalloc(nPoints * 2 * sizeof(float));
	if (!S)
		return stm32ipl_err_OutOfMemory;

	V = xalloc(nPoints * nPoints * sizeof(float));
	if (!V)
		return stm32ipl_err_OutOfMemory;

	array2 = xalloc(M * sizeof(float));
	if (!array2)
		return stm32ipl_err_OutOfMemory;

	circle = xalloc(M * 2 * sizeof(float));
	if (!circle)
		return stm32ipl_err_OutOfMemory;

	points = xalloc(M * sizeof(point_t));
	if (!points)
		return stm32ipl_err_OutOfMemory;

	stop = 2 * PI;
	step = (stop - start) / (M - 1);

	for (uint16_t i = 0; i < nPoints; i++) {
		sum_x += x[i];
		sum_y += y[i];
	}

	mean_x = sum_x / nPoints;
	mean_y = sum_y / nPoints;

	for (uint16_t i = 0; i < nPoints; i++) {
		xx[i] = x[i] - mean_x;
		yy[i] = y[i] - mean_y;
	}

	STM32Ipl_Svd(xx, yy, nPoints, U, S, V);

	step = (stop - start) / (M - 1);

	for (uint16_t i = 0; i < M; i++) {
		array2[i] = start + (i * step);
		circle[i] = cos(array2[i]);
		circle[i + M] = sin(array2[i]);
	}

	for (uint16_t i = 0; i < 2; i++) {
		transform[i * 2] = sqrt(2.0f / nPoints) * U[i * 2] * S[0];
		transform[i * 2 + 1] = sqrt(2.0f / nPoints) * U[i * 2 + 1] * S[9];
	}

	transform[1] *= -1;
	transform[3] *= -1;

	for (int i = 0; i < M; i++) {
		for (int j = 0; j < 2; j++) {
			float acc = 0.0f;

			for (int k = 0; k < 2; k++) {
				acc += transform[j * 2 + k] * circle[i + (k * M)];
			}

			if (j == 0)
				fit[i] = acc + mean_x;
			else
				fit[i + M] = acc + mean_y;
		}
	}

	for (uint8_t i = 0; i < M; i++) {
		point.x = abs(floor(fit[i]));
		point.y = abs(floor(fit[i + M]));
		points[i] = point;
	}

	point_min_area_rectangle(points, min_corners, M);

	STM32Ipl_EnclosingEllipse(min_corners, 4, &center, &dmax, &dmin, &rot);

	STM32Ipl_GetRegressionPoints(&line_points, points, M, false);

	dh = line_points.line.y2 - line_points.line.y1;
	dl = line_points.line.x2 - line_points.line.x1;

	ipo = sqrt(pow(dh, 2) + pow(dl, 2));
	angle = asin(dh / ipo);
	rot = IM_RAD2DEG(angle);

	ellipse->center.x = center.x;
	ellipse->center.y = center.y;
	ellipse->radius_x = (int16_t)dmax;
	ellipse->radius_y = (int16_t)dmin;
	ellipse->rotation = (int16_t)rot;

	xfree(xx);
	xfree(yy);
	xfree(S);
	xfree(V);
	xfree(array2);
	xfree(circle);
	xfree(points);

	return stm32ipl_err_Ok;
}

/**
 * @brief Calculate the perimeter using points
 * @param points	inputs points
 * @param count		number of inputs points
 * @param is_closed	if true calculate a the closure
 * @param perimeter	output
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ArcLength(const point_t *points, uint16_t count, bool is_closed, float *perimeter)
{

	if (points == NULL || count <= 1) {
		*perimeter = 0.;
		return stm32ipl_err_InvalidParameter;
	}

	int last = is_closed ? count - 1 : 0;

	float *ptfx = xalloc(sizeof(float) * count);

	if (ptfx == NULL) {
		return stm32ipl_err_OutOfMemory;
	}

	float *ptfy = xalloc(sizeof(float) * count);

	if (ptfy == NULL) {
		xfree(ptfx);
		return stm32ipl_err_OutOfMemory;
	}

	float prevx = points[last].x;
	float prevy = points[last].y;

	for (int i = 0; i < count; i++) {
		float px = (float)points[i].x;
		float py = (float)points[i].y;
		float dx = px - prevx;
		float dy = py - prevy;
		*perimeter += sqrt(dx * dx + dy * dy);

		prevx = px;
		prevy = py;
	}

	xfree(ptfx);
	xfree(ptfy);
	return stm32ipl_err_Ok;
}

/**
 * @brief Point allocation data values x and y
 *
 * @param x horizontal value of point
 * @param y vertical value of point
 * @return pointer to the point or NULL
 */
point_t* STM32Ipl_PointAlloc(int16_t x, int16_t y)
{
	return point_alloc(x, y);
}

/**
 * @brief Point Release
 * @param ptr pointer to the point to be deallocated
 */
void STM32Ipl_PointFree(point_t *ptr)
{
	xfree(ptr);
	ptr = NULL;
}

/**
 * @brief Point initialization
 * @param ptr pointer to the point to be inizialized (not NULL)
 * @param x horizontal value of point
 * @param y vertical value of point
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_PointInit(point_t *ptr, uint16_t x, uint16_t y)
{
	if (ptr == NULL)
		return stm32ipl_err_InvalidParameter;

	point_init(ptr, x, y);

	return stm32ipl_err_Ok;
}

/**
 * @brief Copy values from a source point to a destination point
 * @param src source point
 * @param dst destination point
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_PointCopy(const point_t *src, point_t *dst)
{
	if (!src || !dst)
		return stm32ipl_err_InvalidParameter;

	point_copy(dst, (point_t*)src);

	return stm32ipl_err_Ok;
}

/**
 * @brief Check if two points are equal using memcmp
 * @param ptr0	first point
 * @param ptr1	second point
 * @return true if equal false otherwise
 */
bool STM32Ipl_PointEqualFast(const point_t *ptr0, const point_t *ptr1)
{
	if (!ptr0 || !ptr1)
		return false;
	if (ptr0 == ptr1)
		return true;

	return point_equal_fast((point_t*)ptr0, (point_t*)ptr1);
}

/**
 * @brief Check if two points are equal using the value of points
 * @param ptr0	first point
 * @param ptr1	second point
 * @return true if equal false otherwise
 */
bool STM32Ipl_PointEqual(const point_t *ptr0, const point_t *ptr1)
{
	if (!ptr0 || !ptr1)
		return false;
	if (ptr0 == ptr1)
		return true;

	return point_equal((point_t*)ptr0, (point_t*)ptr1);
}

/**
 * @brief distance from two points.
 * The distance between \f$(x_1,y_1)\f$ and \f$(x_2,y_2)\f$ is \f$\sqrt{(x_2-x_1)^2+(y_2-y_1)^2}\f$.
 * @param ptr0 first point
 * @param ptr1 second point
 * @param distance result
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_PointDistance(const point_t *ptr0, const point_t *ptr1, float *distance)
{
	if (!ptr0 || !ptr1 || !distance)
		return stm32ipl_err_InvalidParameter;

	*distance = point_distance((point_t*)ptr0, (point_t*)ptr1);

	return stm32ipl_err_Ok;
}

/**
 * @brief quadrance from two points.
 * The quadrance between \f$(x_1,y_1)\f$ and \f$(x_2,y_2)\f$ is \f$(x_2-x_1)^2+(y_2-y_1)^2\f$.
 * @param ptr0 first point
 * @param ptr1 second point
 * @param quadrance result
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_PointQuadrance(const point_t *ptr0, const point_t *ptr1, int *quadrance)
{
	if (!ptr0 || !ptr1 || !quadrance)
		return stm32ipl_err_InvalidParameter;

	*quadrance = point_quadrance((point_t*)ptr0, (point_t*)ptr1);

	return stm32ipl_err_Ok;
}

/**
 * @brief Rotate point(x,y) of degree and with point(center_x,center_y) centre of rotation.
 * @param x horizontal value of point
 * @param y vertical value of point
 * @param degree positive value of rotation in degree
 * @param center_x horizontal value of centre of rotation
 * @param center_y vertical value of centre of rotation
 * @param new_x horizontal value of rotated point
 * @param new_y vertical value of rotated point
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_PointRotate(int16_t x, int16_t y, uint16_t degree, int16_t center_x, int16_t center_y,
		int16_t *new_x, int16_t *new_y)
{
	if (!new_x || !new_y)
		return stm32ipl_err_InvalidParameter;

	point_rotate(x, y, IM_DEG2RAD(degree), center_x, center_y, new_x, new_y); // or STM32IPL_DEG2RAD ?? TODO RB

	return stm32ipl_err_Ok;

}

/**
 * @brief Identify the min area rectangle using corners[corners_len] and set in new_corners[4] the four coordinate of the finded rectagle.
 * @param corners source corners
 * @param new_corners destination corners (four points)
 * @param corners_len length of source corners
 * @return stm32ipl_err_Ok on success
 * @note Corners need to be sorted!
 */
stm32ipl_err_t STM32Ipl_PointMinAreaRectangle(const point_t *corners, point_t *new_corners, uint16_t corners_len)
{
	if (!corners || !new_corners)
		return stm32ipl_err_InvalidParameter;

	point_min_area_rectangle((point_t*)corners, new_corners, corners_len);

	return stm32ipl_err_Ok;
}


// FIXME: meglio ritornare stm32ipl_err_t ?
/**
 * @brief Clipping line in a rectangle area Liang-Barsky line clipping
 * @param l the line
 * @param x horizontal start point of rectangle
 * @param y vertical start point of rectangle
 * @param w	width of the rectangle
 * @param h height of the rectangle
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
bool STM32Ipl_ClipLine(line_t *l, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
	if (!l)
		return false;
	return lb_clip_line(l, x, y, w, h);
}

// FIXME: meglio ritornare stm32ipl_err_t e il risultato come argomento ?
/**
 * @brief Get if a pixel/point into an image is masked or not
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param ptr Source image
 * @param x horizontal value of point
 * @param y vertical value of point
 * @return true only pixels is masked. See note for more details.
 * @note for Binary, 1 mask 0 not mask.
 * 		 for Grayscale, 1 if the value is > (COLOR_GRAYSCALE_MAX - COLOR_GRAYSCALE_MIN) / 2) + COLOR_GRAYSCALE_MIN.
 * 		 for RGB565, 1 if the Y value is > (COLOR_Y_MAX - COLOR_Y_MIN) / 2) + COLOR_Y_MIN)
 * 		 for RGB888, 1 if the Y value is > (COLOR_Y_MAX - COLOR_Y_MIN) / 2) + COLOR_Y_MIN)
 */
bool STM32Ipl_GetMaskPixel(const image_t *ptr, uint16_t x, uint16_t y)
{
	if (!ptr)
		return false;

	return image_get_mask_pixel((image_t*)ptr, x, y);
}

/**
 * @brief This function should be called on an ROI detected with the eye Haar cascade.
 * The supported format is Grayscale.
 * @param src 	Image
 * @param iris 	center of iris
 * @param roi  	region of interest to find iris.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_FindIris(const image_t *src, point_t *iris, const rectangle_t *roi)
{
	STM32IPL_CHECK_VALID_IMAGE(src)

	/* check format */
	if (src->bpp != IMAGE_BPP_GRAYSCALE)
		return stm32ipl_err_UnsupportedFormat;

	if (!iris || !roi)
		return stm32ipl_err_InvalidParameter;

	imlib_find_iris((image_t*)src, iris, (rectangle_t*)roi);

	return stm32ipl_err_Ok;
}

#ifdef IMLIB_ENABLE_HOG
/**
 * @brief Replaces the pixels in the ROI with HOG (histogram of orientated graidients) lines.
 * @param src Grayscale Image
 * @param roi region-of-interest rectangle, if NULL it is equal to the image rectangle.
 * 			  Only pixels within the roi are operated on.
 * @param cell_size must be > 0 . To capture large-scale spatial information, increase the cell size.
 * 		  When you increase the cell size, you may lose small-scale detail.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note check the description cell_size. Other paramer to fild hog is the block_size set to cell_size * 2
 * 		 and the numbers of orientation histogram bins set to 9 to default in hog.c file
 */
stm32ipl_err_t STM32Ipl_FindHog(image_t *src, const rectangle_t *roi, uint8_t cell_size)
{
	STM32IPL_CHECK_VALID_IMAGE(src)

	if (!roi || (cell_size < 1))
		return stm32ipl_err_InvalidParameter;

	/* check format */
	if (src->bpp != IMAGE_BPP_GRAYSCALE)
		return stm32ipl_err_UnsupportedFormat;

	imlib_find_hog(src, (rectangle_t*)roi, cell_size);

	return stm32ipl_err_Ok;
}

#endif

/**
 * @brief Get pixel(x,y) value in a image
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img Input image
 * @param x horizontal pixel value
 * @param y vertical pixel value
 * @param p value of the pixel
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
// FIXME: p deve essere uint32_t ?
stm32ipl_err_t STM32Ipl_GetPixel(const image_t *img, uint16_t x, uint16_t y, int *p)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if ((x >= img->w) && (y >= img->h))
		return stm32ipl_err_InvalidParameter;

	switch (img->bpp) {
		case IMAGE_BPP_BINARY: {
			*p = (int)IMAGE_GET_BINARY_PIXEL(img, x, y);
			break;
		}
		case IMAGE_BPP_GRAYSCALE: {
			*p = (int)IMAGE_GET_GRAYSCALE_PIXEL(img, x, y);
			break;
		}
		case IMAGE_BPP_RGB565: {
			*p = (int)IMAGE_GET_RGB565_PIXEL(img, x, y);
			break;
		}
		case IMAGE_BPP_RGB888: {
			rgb888_t pixel888;
			pixel888 = IMAGE_GET_RGB888_PIXEL(img, x, y);
			*p = 0;
			*p = ((pixel888.r << 16) | (pixel888.g << 8) | (pixel888.b));
			//*p = (int) IMAGE_GET_RGB888_PIXEL(img, x, y);
			break;
		}
		default: {
			return stm32ipl_err_InvalidParameter;
			break;
		}
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Get the mean of image data values
 * The supported formats are Grayscale, RGB565, RGB888.
 * @param src	 Input Image
 * @param r_mean r mean value or Grayscale mean value
 * @param g_mean g mean value or Grayscale mean value
 * @param b_mean r mean value or Grayscale mean value
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ImageMean(const image_t *src, int *r_mean, int *g_mean, int *b_mean)
{

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_FORMAT(src, (stm32ipl_if_grayscale | stm32ipl_if_rgb565 | stm32ipl_if_rgb888))

	if (!r_mean || !g_mean || !b_mean)
		return stm32ipl_err_InvalidParameter;

	imlib_image_mean((image_t*)src, r_mean, g_mean, b_mean);

	return stm32ipl_err_Ok;
}

/**
 * @brief Get a standard deviation value of the grayscale image.
 * The supported format is (Grayscale).
 * @param src Input Image
 * @param grayscale_std	standard deviation
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ImageStd(const image_t *src, int16_t *grayscale_std)
{
	STM32IPL_CHECK_VALID_IMAGE(src)
	/* check format */
	if (src->bpp != IMAGE_BPP_GRAYSCALE)
		return stm32ipl_err_UnsupportedFormat;

	if (!grayscale_std)
		return stm32ipl_err_InvalidParameter;

	*grayscale_std = imlib_image_std((image_t*)src);

	return stm32ipl_err_Ok;
}

/**
 * @brief Finds the midpoint of x_div * y_div squares in the image and overwrite the image with it composed of the midpoint of each square.
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img Input Image
 * @param x_div width of kernel
 * @param y_div height of kernel
 * @param bias A bias of 0 returns the min of each area while a bias of 256 returns the max of each area.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note img is replaced with a shrunk one
 */
stm32ipl_err_t STM32Ipl_MidpointPooled(image_t *img, int x_div, int y_div, const int bias)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)

	if ((x_div < 1) || (y_div < 1) || (x_div > img->w) || (y_div > img->h))
		return stm32ipl_err_InvalidParameter;

	if (!((0 <= bias) && (bias <= 256)))
		return stm32ipl_err_InvalidParameter;

	image_t out_img;

	STM32Ipl_AllocData(&out_img, img->w / x_div, img->h / y_div, (image_bpp_t)img->bpp);

	if (out_img.pixels == NULL)
		return stm32ipl_err_OutOfMemory;

	imlib_midpoint_pool(img, &out_img, x_div, y_div, bias);

	STM32Ipl_ReleaseData(img);

	STM32Ipl_Init(img, out_img.w, out_img.h, (image_bpp_t)out_img.bpp, out_img.data);

	return stm32ipl_err_Ok;
}

/**
 * @brief Finds the mean of x_div * y_div squares in the image and overwrite the image with it composed of the mean of each square.
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img Input Image
 * @param x_div width of kernel
 * @param y_div height of kernel
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note img is replaced with a shrunk one
 */
stm32ipl_err_t STM32Ipl_MeanPooled(image_t *img, int x_div, int y_div)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)

	if ((x_div < 1) || (y_div < 1) || (x_div > img->w) || (y_div > img->h))
		return stm32ipl_err_InvalidParameter;

	image_t out_img;
	out_img.w = img->w / x_div;
	out_img.h = img->h / y_div;
	out_img.bpp = img->bpp;

	STM32Ipl_AllocData(&out_img, out_img.w, out_img.h, (image_bpp_t)out_img.bpp);

	if (out_img.pixels == NULL)
		return stm32ipl_err_OutOfMemory;

	imlib_mean_pool(img, &out_img, x_div, y_div);

	STM32Ipl_ReleaseData(img);

	STM32Ipl_Init(img, out_img.w, out_img.h, (image_bpp_t)out_img.bpp, out_img.data);

	return stm32ipl_err_Ok;
}

/**
 * @brief The same as midpoint_pooled but the source isn't replaced
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param src Input Image
 * @param dst Destination Image same type (BINARY / GRAYSCALE / RGB565) as Input Image (No Conversions). width and height must be a fraction of source Image.
 * 			  The (x_div,y_div) values of pooled version function are derived x_div= (src->w / dst->src ) and y_div = (src->w / dst->src )
 * @param bias A bias of 0 returns the min of each area while a bias of 256 returns the max of each area.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_MidpointPool(const image_t *src, image_t *dst, const int bias)
{

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_NOT_RGB888)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((src == dst))
		return stm32ipl_err_InvalidParameter;

	if (STM32Ipl_ImageDataSize(dst) > STM32Ipl_ImageDataSize(src))
		return stm32ipl_err_InvalidParameter;

	if (!((0 <= bias) && (bias <= 256)))
		return stm32ipl_err_InvalidParameter;

	uint16_t x_div = src->w / dst->w;
	uint16_t y_div = src->h / dst->h;

	imlib_midpoint_pool((image_t*)src, dst, x_div, y_div, bias);

	return stm32ipl_err_Ok;
}

/**
 * @brief The same as mean_pooled but the source isn't replaced
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param src Input Image
 * @param dst Destination Image same type (BINARY / GRAYSCALE / RGB565) as Input Image (No Conversions). width and height must be a fraction of source Image.
 * 			  The (x_div,y_div) values of pooled version function are derived x_div= (src->w / dst->src ) and y_div = (src->w / dst->src )
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_MeanPool(const image_t *src, image_t *dst)
{
	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_NOT_RGB888)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((src == dst))
		return stm32ipl_err_InvalidParameter;

	if (STM32Ipl_ImageDataSize(dst) > STM32Ipl_ImageDataSize(src))
		return stm32ipl_err_InvalidParameter;

	imlib_mean_pool((image_t*)src, dst, src->w / dst->w, src->h / dst->h);

	return stm32ipl_err_Ok;
}

/**
 * @brief Runs the mode filter on the image by replacing each pixel with the mode of their neighbours. This method works great on grayscale images. However, on RGB images it creates a lot of artifacts on edges because of the non-linear nature of the operation.
 * The supported formats are Binary, Grayscale, RGB565B, RGB888.
 * @param img Input Image
 * @param ksize is the kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), etc.
 * @param threshold if true will enable adaptive thresholding of the image which sets pixels to one or zero based on a pixel’s brightness in relation to the brightness of the kernel of pixels around them.
 * @param offset if negative value sets more pixels to 1 as you make it more negative while a positive value only sets the sharpest contrast changes to 1
 * @param invert if true set the binary image resulting output
 * @param mask  is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ModeFilter(image_t *img, const int ksize, bool threshold, int offset, bool invert,
		const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	imlib_mode_filter(img, ksize, threshold, offset, invert, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Runs the midpoint filter on the image. This filter finds the midpoint ((max-min)/2) of each pixel neighborhood in the image.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img Input Image
 * @param ksize is the kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), etc.
 * @param bias controls the min/max mixing. 0 for min filtering only, 1.0 for max filtering only
 * @param threshold if true enable adaptive thresholding of the image which sets pixels to one or zero based on a pixel’s brightness in relation to the brightness of the kernel of pixels around them.
 * @param offset  if threshold is true and offset set to a negative value, sets more pixels to 1 as you make it more negative while a positive value only sets the sharpest contrast changes to 1
 * @param invert if threshold is true and invert is true the binary image resulting output is inverted
 * @param mask is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_MidpointFilter(image_t *img, const int ksize, float bias, bool threshold, int offset,
bool invert, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	if (!((0 <= bias) && (bias <= 1)))
		return stm32ipl_err_InvalidParameter;

	imlib_midpoint_filter(img, ksize, bias, threshold, offset, invert, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Convolves the image by a bilateral filter. The bilateral filter smooth the image while keeping edges in the image.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img Input Image
 * @param ksize	is the kernel size. Use 1 (3x3 kernel), 2 (5x5 kernel), etc.
 * @param color_sigma controls how closely colors are matched using the bilateral filter. Increase this to increase color blurring.
 * @param space_sigma controls how closely pixels space-wise are blurred with each other. Increase this to increase pixel blurring.
 * @param threshold if true enable adaptive thresholding of the image which sets pixels to one or zero based on a pixel’s brightness in relation to the brightness of the kernel of pixels around them
 * @param offset  if threshold is true and offset set to a negative value, sets more pixels to 1 as you make it more negative while a positive value only sets the sharpest contrast changes to 1
 * @param invert if threshold is true and invert is true the binary image resulting output is inverted
 * @param mask is another image to use as a pixel level mask for the operation. The mask should be an image with just black or white pixels and should be the same size as the image being operated on. Only pixels set in the mask are modified
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note here it's possible view the example code for it's use @link stm32ipl_demo_smoothing.c @endlink
 */
stm32ipl_err_t STM32Ipl_BilateralFilter(image_t *img, const int ksize, float color_sigma, float space_sigma,
bool threshold, int offset, bool invert, const image_t *mask)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (mask) {
		STM32IPL_CHECK_VALID_IMAGE(mask)
		STM32IPL_CHECK_FORMAT(mask, STM32IPL_IF_ALL)
		STM32IPL_CHECK_SAME_RESOLUTION(img, mask)
	}

	imlib_bilateral_filter((image_t*)img, ksize, color_sigma, space_sigma, threshold, offset, invert, (image_t*)mask);

	return stm32ipl_err_Ok;
}

/**
 * @brief Re-projects an image from Cartesian coordinates to log polar coordinates.
 * Log polar re-projection turns rotation of an image into x-translation and scaling/zooming into y-translation.
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img Input Image
 * @param reverse if true re-project in the opposite direction.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Logpolar(image_t *img, bool reverse)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)

	imlib_logpolar(img, false, reverse);

	return stm32ipl_err_Ok;
}

/**
 * @brief Re-projects an image from Cartesian coordinates to linear polar coordinates.
 * Linear polar re-projection turns rotation of an image into x-translation.
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img Input Image
 * @param reverse if true re-project in the opposite direction.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_Linpolar(image_t *img, bool reverse)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)

	imlib_logpolar(img, true, reverse);

	return stm32ipl_err_Ok;
}

/**
 * @brief Structural Similarity (SSIM) algorithm compares 8x8 blocks of pixels between two images to determine a similarity score between two images.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param img Input Image
 * @param other same (width, height, bpp) as img or NULL
 * @param scalar if other is NULL use this scalar to found the similarity
 * @param avg average
 * @param std standard deviation
 * @param min minimum
 * @param max maximum
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GetSimilarity(const image_t *img, const image_t *other, int scalar, float *avg, float *std,
		float *min, float *max)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(other)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (!IM_EQUAL(img, other))
		return stm32ipl_err_InvalidParameter;

	imlib_get_similarity((image_t*)img, NULL, (image_t*)other, scalar, avg, std, min, max);

	return stm32ipl_err_Ok;
}

/**
 * @brief Use the histogram (L,A,B) to extract the percentile of L,A,B
 * @param ptr Input histogram
 * @param bpp Image format used to built the histogram; supported formats are Binary, Grayscale, RGB565, RGB888)
 * @param out percentile
 * @param percentile if you pass in 0.1 this method will tell you (going from left-to-right in the histogram) what bin when summed into an accumulator caused the accumulator to cross 0.1.
 *        This is useful to determine min (with 0.1) and max (with 0.9) of a color distribution without outlier effects ruining your results for adaptive color tracking.
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GetPercentile(const histogram_t *ptr, image_bpp_t bpp, percentile_t *out, float percentile)
{
	if (!out || !ptr)
		return stm32ipl_err_InvalidParameter;

	image_t img;
	img.bpp = bpp;

	STM32IPL_CHECK_FORMAT(&img, STM32IPL_IF_ALL)

	if ((bpp == IMAGE_BPP_BINARY || bpp == IMAGE_BPP_GRAYSCALE) && !ptr->LBins)
		return stm32ipl_err_InvalidParameter;
	else
		if ((bpp == IMAGE_BPP_RGB565 || bpp == IMAGE_BPP_RGB888) && !ptr->LBins && !ptr->ABins && !ptr->BBins)
			return stm32ipl_err_InvalidParameter;

	imlib_get_percentile(out, bpp, (histogram_t*)ptr, percentile);

	return stm32ipl_err_Ok;
}

/**
 * @brief Uses Otsu method to compute the optimal threshold values that split the histogram into two halves for each channel of the histogram.
 * @param ptr Input histogram
 * @param bpp Image format used to built the histogram; supported formats are Binary, Grayscale, RGB565, RGB888)
 * @param out L, A, B thresholds
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_GetThreshold(const histogram_t *ptr, image_bpp_t bpp, threshold_t *out)
{
	if (!out || !ptr)
		return stm32ipl_err_InvalidParameter;

	image_t img;
	img.bpp = bpp;

	STM32IPL_CHECK_FORMAT(&img, STM32IPL_IF_ALL)

	imlib_get_threshold(out, bpp, (histogram_t*)ptr);

	return stm32ipl_err_Ok;
}

/**
 * @brief Finds all infinite lines in the image using the hough transform. Returns a list of line (find_lines_list_lnk_data_t)
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param out list of find_lines_list_lnk_data_t found
 * @param ptr Input Image
 * @param roi is the region-of-interest rectangle. If NULL, it is equal to the image rectangle.  Only pixels within the roi are operated on.
 * @param x_stride is the number of x pixels to skip when doing the hough transform. Only increase this if lines you are searching for are large and bulky.
 * @param y_stride is the number of y pixels to skip when doing the hough transform. Only increase this if lines you are searching for are large and bulky.
 * @param threshold  controls what lines are detected from the hough transform.
 * 					 Only lines with a magnitude greater than or equal to threshold are returned.
 * 					 The right value of threshold for your application is image dependent.
 * 					 Note that the magnitude of a line is the sum of all sobel filter magnitudes of pixels that make up that line.
 * @param theta_margin controls the merging of detected lines.
 * @param rho_margin  controls the merging of detected lines.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note Lines which are theta_margin degrees apart and rho_margin rho apart are merged.
 */
stm32ipl_err_t STM32Ipl_FindLines(list_t *out, const image_t *ptr, const rectangle_t *roi, uint8_t x_stride,
		uint8_t y_stride, uint32_t threshold, uint8_t theta_margin, uint8_t rho_margin)
{
	STM32IPL_CHECK_VALID_IMAGE(ptr)
	STM32IPL_CHECK_FORMAT(ptr, STM32IPL_IF_ALL)

	if (!out || (x_stride < 1) || (y_stride < 1))
		return stm32ipl_err_InvalidParameter;

	rectangle_t _roi = { 0, 0, 0, 0 };
	if (roi == NULL) {
		_roi.w = ptr->w;
		_roi.h = ptr->h;
	}

	imlib_find_lines(out, (image_t*)ptr, ((roi == NULL) ? &_roi : (rectangle_t*)roi), x_stride, y_stride, threshold,
			theta_margin, rho_margin);

	return stm32ipl_err_Ok;
}

/**
 * @brief Calculate the line length
 * @param lnk_data Input line
 * @param length length
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_LineLength(const find_lines_list_lnk_data_t *lnk_data, uint32_t *length)
{
	if (!lnk_data || !length)
		return stm32ipl_err_InvalidParameter;

	int32_t x_diff = lnk_data->line.x2 - lnk_data->line.x1;
	int32_t y_diff = lnk_data->line.y2 - lnk_data->line.y1;
	*length = fast_roundf(fast_sqrtf((x_diff * x_diff) + (y_diff * y_diff)));

	return stm32ipl_err_Ok;
}

/**
 * @brief Find the translation offset of the this image from the template.
 * This method can be used to do optical flow and returns a displacement (x_traslation, y_traslation, rotation, scale) object with the results of the displacement calculation using phase correlation.
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img Input Image
 * @param roi is the region-of-interest rectangle to work in. If NULL, it is equal to the img rectangle.
 * @param template Input Image
 * @param roi_template is the region-of-interest rectangle to work in. If NULL, it is equal to the template rectangle.
 * @param logpolar if false compute the x/y traslation between two images, if true find rotation and scale changes between the two images.
 * @param fix_rotation_scale set to false.
 * @param x_translation is the x translation between two images
 * @param y_translation is the y translation between two images
 * @param rotation Returns the rotation in radians between two images.
 * @param scale is the scale between two images
 * @param response is the quality of the results of displacement matching between two images.
 * 				   The value change from 0 to 1 and a response less than 0.1 is likely noise.
 * @return	stm32ipl_err_Ok on success, error otherwise
 * @note OpenMv documentation not present fix rotation and set to false in wrapper
 */
stm32ipl_err_t STM32Ipl_FindDisplacement(const image_t *img, const rectangle_t *roi, const image_t *template,
		const rectangle_t *roi_template, bool logpolar, bool fix_rotation_scale, float *x_translation,
		float *y_translation, float *rotation, float *scale, float *response)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_VALID_IMAGE(template)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_NOT_RGB888)
	STM32IPL_CHECK_FORMAT(template, STM32IPL_IF_NOT_RGB888)

	if (!x_translation || !y_translation || !rotation || !scale || !response)
		return stm32ipl_err_InvalidParameter;

	rectangle_t _roi = { 0, 0, 0, 0 };
	if (roi == NULL) {
		_roi.w = img->w;
		_roi.h = img->h;
	} else
		rectangle_copy(&_roi, (rectangle_t*)roi);

	rectangle_t _roi_template = { 0, 0, 0, 0 };

	if (roi_template == NULL) {
		_roi_template.w = template->w;
		_roi_template.h = template->h;
	} else
		rectangle_copy(&_roi_template, (rectangle_t*)roi_template);

	if ((_roi.w != _roi_template.w) || (_roi.h != _roi_template.h))
		return stm32ipl_err_InvalidParameter;

	imlib_phasecorrelate((image_t*)img, (image_t*)template, &_roi, &_roi_template, logpolar, fix_rotation_scale,
			x_translation, y_translation, rotation, scale, response);

	return stm32ipl_err_Ok;
}

/**
 * @brief Zeros a rectangle part of the image
 * The supported formats are (Binary, Grayscale, RGB565, RGB888).
 * @param img Input Image
 * @param x horizontal start point of rectangle
 * @param y vertical start point of rectangle
 * @param w width of rectangle
 * @param h height of rectangle
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ImageMaskRectangle(image_t *img, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	image_t temp;
	temp.w = img->w;
	temp.h = img->h;
	temp.bpp = IMAGE_BPP_BINARY;
	temp.data = xalloc0(image_size(&temp));
	if (temp.data == NULL)
		return stm32ipl_err_OutOfMemory;

	imlib_draw_rectangle(&temp, x, y, w, h, -1, 0, true);
	imlib_zero(img, &temp, true);
	STM32Ipl_ReleaseData(&temp);

	return stm32ipl_err_Ok;
}

/**
 * @brief Zeros a circular part of the image
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img Input Image
 * @param cx x center of the ellipse
 * @param cy y center of the ellipse
 * @param radius radius of the circle
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ImageMaskCircle(image_t *img, uint16_t cx, uint16_t cy, uint16_t radius)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	image_t temp;
	temp.w = img->w;
	temp.h = img->h;
	temp.bpp = IMAGE_BPP_BINARY;
	temp.data = xalloc0(image_size(&temp));
	if (temp.data == NULL)
		return stm32ipl_err_OutOfMemory;

	imlib_draw_circle(&temp, cx, cy, radius, -1, 0, true);
	imlib_zero(img, &temp, true);

	STM32Ipl_ReleaseData(&temp);

	return stm32ipl_err_Ok;
}

/**
 * @brief Zeros an ellipsed shaped part of the image.
 * The supported formats are (Binary, Grayscale, RGB565).
 * @param img Input Image
 * @param cx x center of the ellipse
 * @param cy y center of the ellipse
 * @param xradius size of radius in x direction
 * @param yradius size of radius in y direction
 * @param rotation rotation degree of ellipse [0,360]
 * @return	stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_ImageMaskEllipse(image_t *img, uint16_t cx, uint16_t cy, uint16_t xradius, uint16_t yradius,
		uint16_t rotation)
{
	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, STM32IPL_IF_ALL)

	if (rotation > 360)
		return stm32ipl_err_InvalidParameter;

	image_t temp;
	temp.w = img->w;
	temp.h = img->h;
	temp.bpp = IMAGE_BPP_BINARY;
	temp.data = xalloc0(image_size(&temp));
	if (temp.data == NULL)
		return stm32ipl_err_OutOfMemory;

	imlib_draw_ellipse(&temp, cx, cy, xradius, yradius, rotation, -1, 0, true);
	imlib_zero(img, &temp, true);

	STM32Ipl_ReleaseData(&temp);
	return stm32ipl_err_Ok;
}
