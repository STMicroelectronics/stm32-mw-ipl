/**
 ******************************************************************************
 * @file   stm32ipl_image_io_jpg_sw.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - JPEG SW codec
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

#include "stm32ipl_image_io_jpg_sw.h"

#ifndef STM32IPL_USE_HW_JPEG_CODEC

#include "jpeglib.h"

#ifdef STM32IPL_PERF
#include "perf.h"
extern perfTimes_t perf;
#endif /* STM32IPL_PERF */


typedef struct _RGB888_t
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGB888_t;

typedef uint16_t RGB565_t;

typedef void (*ConvertLineFunction)(const uint8_t* src, uint8_t *dst, uint32_t width);


void ConvertLineRGB888ToRGB565(const uint8_t* src, uint8_t *dst, uint32_t width)
{
	RGB888_t *rgb888 = (RGB888_t*)src;
	RGB565_t *rgb565 = (uint16_t*)dst;

	for (uint32_t i = 0; i < width; i++) {
		uint16_t b = ( rgb888[i].b >> 3) & 0x1f;
		uint16_t g = ((rgb888[i].g >> 2) & 0x3f) << 5;
		uint16_t r = ((rgb888[i].r >> 3) & 0x1f) << 11;

		*rgb565++ = (RGB565_t)(r | g | b);
	}
}


void ConvertLineRGB565ToRGB888(const uint8_t* src, uint8_t *dst, uint32_t width)
{
	RGB565_t *rgb565 = (RGB565_t*)src;
	RGB888_t *rgb888 = (RGB888_t*)dst;

	for (uint32_t i = 0; i < width; i++)
	{
		RGB565_t val = *rgb565++;

		uint32_t r = (val & 0xF800) >> 11;
		uint32_t g = (val & 0x07E0) >> 5;
		uint32_t b = (val & 0x001F);

		rgb888[i].r = (r << 3) | (r >> 2);
		rgb888[i].g = (g << 2) | (g >> 4);
		rgb888[i].b = (b << 3) | (b >> 2);
	}
}


void ConvertLineGrayToRGB565(const uint8_t* src, uint8_t *dst, uint32_t width)
{
	uint8_t *gray = (uint8_t *)src;
	RGB565_t *rgb565 = (RGB565_t*)dst;

	for (uint32_t i = 0; i < width; i++)
	{
		uint8_t val = *gray++;

		RGB565_t b = ( val >> 3) & 0x1f;
		RGB565_t g = ((val >> 2) & 0x3f) << 5;
		RGB565_t r = b << 11;

		*rgb565++ = (RGB565_t)(r | g | b);
	}
}


void ConvertLineGrayToGray(const uint8_t* src, uint8_t *dst, uint32_t width)
{
	memcpy(dst, src, width);
}


/* Read and decode a JPEG file and return it into a image structure pointed by img;
 * the libJPEG software decoder is used. RGB and grayscale formats are supported.
 * img: pointer to the decoded image; image data is allocated internally, so it is up to
 * the caller to release it when done with it.
 * fp: the pointer to the file object.
 * return stm32ipl_err_Ok on success, error otherwise.
 * */
stm32ipl_err_t readJPEGSW(image_t *img, FIL* fp)
{
	struct jpeg_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;
	JSAMPROW buffer[1] = { 0 };
	uint8_t *auxLine;
	uint8_t *imgData;
	uint8_t *imgLine;
	uint32_t bpp = 0;
	ConvertLineFunction convertFn = 0;

	if (!img || !fp)
		return stm32ipl_err_InvalidParameter;

	image_init(img, 0, 0, 0, 0);

	if (f_lseek(fp, 0) != FR_OK)
		return stm32ipl_err_SeekingFile;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);
	cinfo.dct_method = JDCT_FLOAT;
	jpeg_start_decompress(&cinfo);

	switch (cinfo.out_color_space)
	{
		case JCS_RGB:
			convertFn = ConvertLineRGB888ToRGB565;
			bpp = IMAGE_BPP_RGB565;
			break;

		case JCS_GRAYSCALE:
			convertFn = ConvertLineGrayToGray;
			bpp = IMAGE_BPP_GRAYSCALE;
			break;

		case JCS_YCbCr:
		case JCS_CMYK:
		case JCS_YCCK:
		case JCS_UNKNOWN:
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return stm32ipl_err_UnsupportedFormat;
	}

	auxLine = xalloc(cinfo.output_width * cinfo.out_color_components);
	if (!auxLine)
	{
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return stm32ipl_err_OutOfMemory;
	}

	imgData = xalloc(STM32Ipl_DataSize(cinfo.output_width, cinfo.output_height, bpp));
	if (!imgData)
	{
		xfree(auxLine);
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return stm32ipl_err_OutOfMemory;
	}

	buffer[0] = auxLine;
	imgLine = imgData;

	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, buffer, 1);
		convertFn(auxLine, imgLine, cinfo.output_width);
		imgLine += cinfo.output_width * bpp;
	}

	image_init(img, cinfo.output_width, cinfo.output_height, bpp, imgData);

	xfree(auxLine);

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return stm32ipl_err_Ok;
}


/* Encode and write the image into a JPEG file; the libJPEG software encoder is used;
 * RGB565 and grayscale formats are supported.
 * img: pointer to the image to be encoded and saved.
 * fp: the pointer to the file object.
  * chromaSubsampling: the chroma subsampling; 4:4:4, 4:2:2, 4:2:0 are supported.
  * quality: the quality value used by the encoder (0-100).
 */
static stm32ipl_err_t jpeg_encode(const image_t *img, FIL *fp, uint32_t chromaSubsampling, uint32_t quality)
{
	struct jpeg_error_mgr jerr;
	struct jpeg_compress_struct cinfo;
	JSAMPROW buffer[1] = { 0 };
	uint8_t *auxLine;
	uint8_t *imgLine;
	ConvertLineFunction convertFn;

	if ((img->bpp != IMAGE_BPP_RGB565) && (img->bpp != IMAGE_BPP_GRAYSCALE))
		return stm32ipl_err_UnsupportedFormat;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width  = img->w;
	cinfo.image_height = img->h;

	switch (img->bpp)
	{
		case IMAGE_BPP_RGB565:
			convertFn = ConvertLineRGB565ToRGB888;
			cinfo.input_components = 3;
			cinfo.in_color_space   = JCS_RGB;
			break;

		case IMAGE_BPP_GRAYSCALE:
			convertFn = ConvertLineGrayToGray;
			cinfo.input_components = 1;
			cinfo.in_color_space   = JCS_GRAYSCALE;
			break;

		default:
			jpeg_destroy_compress(&cinfo);
			return stm32ipl_err_UnsupportedFormat;
	}

	switch (chromaSubsampling)
	{
		case STM32IPL_JPEG_444_SUBSAMPLING:
			cinfo.comp_info[0].h_samp_factor = 1;
			cinfo.comp_info[0].v_samp_factor = 1;
			cinfo.comp_info[1].h_samp_factor = 1;
			cinfo.comp_info[1].v_samp_factor = 1;
			cinfo.comp_info[2].h_samp_factor = 1;
			cinfo.comp_info[2].v_samp_factor = 1;
			break;

		case STM32IPL_JPEG_420_SUBSAMPLING:
		     cinfo.comp_info[0].h_samp_factor = 2;
		     cinfo.comp_info[0].v_samp_factor = 2;
		     cinfo.comp_info[1].h_samp_factor = 1;
		     cinfo.comp_info[1].v_samp_factor = 1;
		     cinfo.comp_info[2].h_samp_factor = 1;
		     cinfo.comp_info[2].v_samp_factor = 1;
			break;

		case STM32IPL_JPEG_422_SUBSAMPLING:
	         cinfo.comp_info[0].h_samp_factor = 2;
	         cinfo.comp_info[0].v_samp_factor = 1;
	         cinfo.comp_info[1].h_samp_factor = 1;
	         cinfo.comp_info[1].v_samp_factor = 1;
	         cinfo.comp_info[2].h_samp_factor = 1;
	         cinfo.comp_info[2].v_samp_factor = 1;
			break;

		default:
			jpeg_destroy_compress(&cinfo);
			return stm32ipl_err_UnsupportedFormat;
	}

	auxLine = xalloc(img->w * cinfo.input_components);
	if (!auxLine)
	{
		jpeg_destroy_compress(&cinfo);
		return stm32ipl_err_OutOfMemory;
	}

	jpeg_set_defaults(&cinfo);

	cinfo.dct_method = JDCT_FLOAT;

	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	buffer[0] = auxLine;
	imgLine = img->data;

	while (cinfo.next_scanline < cinfo.image_height)
	{
#ifdef STM32IPL_PERF
		uint32_t tickStart = HAL_GetTick();
#endif /* STM32IPL_PERF */
		convertFn(imgLine, auxLine, cinfo.image_width);
#ifdef STM32IPL_PERF
		perf.encConv += HAL_GetTick() - tickStart;
#endif /* STM32IPL_PERF */
		imgLine += cinfo.image_width * img->bpp;

		jpeg_write_scanlines(&cinfo, buffer, 1);
	}

	xfree(auxLine);

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	return stm32ipl_err_Ok;
}


/* Write the given image to a JPEG file
 * img: pointer to the image to be encoded and saved;
 * RGB565 and grayscale formats are supported.
 * filename: the name of the output file
 * return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t saveJPEGSW(const image_t *img, const char *filename)
{
	stm32ipl_err_t res;
	FIL fp;

	if (!img || !filename)
		return stm32ipl_err_InvalidParameter;

	uint32_t tickStart = HAL_GetTick();
	if (f_open(&fp, (const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return stm32ipl_err_OpeningFile;
#ifdef STM32IPL_PERF
	perf.encFileIO += HAL_GetTick() - tickStart;
#endif /* STM32IPL_PERF */
	res = jpeg_encode(img, &fp, STM32IPL_JPEG_422_SUBSAMPLING, STM32IPL_JPEG_QUALITY);
#ifdef STM32IPL_PERF
	tickStart = HAL_GetTick();
#endif /* STM32IPL_PERF */
	f_close(&fp);
#ifdef STM32IPL_PERF
	perf.encFileIO += HAL_GetTick() - tickStart;
#endif /* STM32IPL_PERF */
	return res;
}

#endif /* STM32IPL_USE_HW_JPEG_CODEC */
