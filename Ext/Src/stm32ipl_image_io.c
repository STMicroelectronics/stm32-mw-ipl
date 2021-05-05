/**
 ******************************************************************************
 * @file   stm32ipl_image_io.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - image read/write functions
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

#include "stm32ipl_image_io.h"

#ifdef STM32IPL_USE_HW_JPEG_CODEC
#include "stm32ipl_image_io_jpg_hw.h"
#else
#include "stm32ipl_image_io_jpg_sw.h"
#endif

#include <ctype.h>

#define BI_RGB              0
#define BI_RLE8             1
#define BI_RLE4             2
#define BI_BITFIELDS        3

#define RGB555_RED_MASK     0x7C00
#define RGB555_GREEN_MASK   0x03E0
#define RGB555_BLUE_MASK    0x001F

#define RGB565_RED_MASK     0xF800
#define RGB565_GREEN_MASK   0x07E0
#define RGB565_BLUE_MASK    0x001F


typedef enum _ImageFileFormatType {
	iplFileFormatUnknown,
	iplFileFormatBMP,
	iplFileFormatPNM,
	iplFileFormatJPG,
} ImageFileFormatType;

typedef struct _RGB888_t {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGB888_t;


static stm32ipl_err_t read_bmp(image_t *img, FIL *fp);
static stm32ipl_err_t read_pnm(image_t *img, FIL *fp);
static stm32ipl_err_t read_jpg(image_t *img, FIL *fp);
static stm32ipl_err_t save_bmp(const image_t *img, const char *filename);
static stm32ipl_err_t save_pnm(const image_t *img, const char *filename, bool ascii);
static stm32ipl_err_t save_jpg(const image_t *img, const char *filename);


/* Return the image file format by analyzing the file extension.
 * BMP, PPM, PGM, JPEG formats are supported.
 * filename: the name of the input file.
 * return the image file format.
 */
static ImageFileFormatType get_image_file_format(const char *filename)
{
	ImageFileFormatType format = iplFileFormatUnknown;
	size_t len;
	char* uprFilename;
	char *ptr;

	if (!filename)
		return iplFileFormatUnknown;

	len = strlen(filename);

	/* Convert to upper case. */
	uprFilename = xalloc(len);
	strcpy(uprFilename, filename);
        
	for (size_t i = 0; i < len; i++)
		uprFilename[i] = toupper(uprFilename[i]);

	ptr = uprFilename + len;

	if (len >= 5) {
		if ((ptr[-1] == 'G') && (ptr[-2] == 'E') && (ptr[-3] == 'P') && (ptr[-4] == 'J') && (ptr[-5] == '.'))
			format = iplFileFormatJPG;
	}
	if (len >= 4) {
		if ((ptr[-1] == 'G') && (ptr[-2] == 'P') && (ptr[-3] == 'J') && (ptr[-4] == '.'))
			format = iplFileFormatJPG;
		else
		if ((ptr[-1] == 'P') && (ptr[-2] == 'M') && (ptr[-3] == 'B') && (ptr[-4] == '.'))
			format = iplFileFormatBMP;
		else
		if ((ptr[-1] == 'M') && (ptr[-2] == 'P') && (ptr[-3] == 'P') && (ptr[-4] == '.'))
			format = iplFileFormatPNM;
		else
		if ((ptr[-1] == 'M') && (ptr[-2] == 'G') && (ptr[-3] == 'P') && (ptr[-4] == '.'))
			format = iplFileFormatPNM;
	}

	xfree(uprFilename);

	return format;
}


/**
 * @brief Read image file; supported formats are: BMP, JPG, PNM (PPM, PGM)
 * Compressed BMP files are not supported. Depending on the configuration
 * file (stm32ipl_conf.h) the SW or the HW JPEG decoder is used.
 * @param img		The image read. The pixel data is allocated internally
 * and must be released with STM32Ipl_ReleaseData(); assuming that input
 * img->data is null.
 * @param filename	The name of the input file
 * @return stm32ipl_err_Ok on success, errors otherwise
 */
stm32ipl_err_t STM32Ipl_ReadImage(image_t* img, const char* filename)
{
	FIL fp;
	uint32_t bytesRead = 0;
	uint8_t magic[2];
	stm32ipl_err_t res;

	const uint8_t bmp[2] = {0x42, 0x4D}; /* BM */
	const uint8_t p2[2]  = {0x50, 0x32}; /* P2 */
	const uint8_t p3[2]  = {0x50, 0x33}; /* P3 */
	const uint8_t p5[2]  = {0x50, 0x35}; /* P5 */
	const uint8_t p6[2]  = {0x50, 0x36}; /* P6 */
	const uint8_t jpg[2] = {0xFF, 0xD8}; /* FFD8 */

	if (!img || !filename)
		return stm32ipl_err_InvalidParameter;

	if (f_open(&fp, (const TCHAR*)filename, FA_OPEN_EXISTING | FA_READ) != FR_OK)
		return stm32ipl_err_OpeningFile;

	if ((f_read(&fp, magic, 2, (UINT*)&bytesRead) != FR_OK) || bytesRead != 2) {
		f_close(&fp);
		return stm32ipl_err_ReadingFile;
	}

	if (memcmp(bmp, magic, 2) == 0)
		res = read_bmp(img, &fp);
	else
	if ((memcmp(p2, magic, 1) == 0) &&
		((memcmp(p2, magic, 2) == 0) || (memcmp(p3, magic, 2) == 0) ||
		(memcmp(p5, magic, 2) == 0) || (memcmp(p6, magic, 2) == 0)))
			res = read_pnm(img, &fp);
	else
	if (memcmp(jpg, magic, 2) == 0)
		res = read_jpg(img, &fp);
	else
		res = stm32ipl_err_UnsupportedFormat;

	f_close(&fp);

	return res;
}




/* Read BMP image file.
 * img: The image read. The pixel data is allocated internally
 * and must be released with STM32Ipl_ReleaseData(); assuming that input
 * img->data is null.
 * fp Pointer to the input file structure.
 * return stm32ipl_err_Ok on success, errors otherwise
 */
static stm32ipl_err_t read_bmp(image_t *img, FIL *fp)
{
	uint32_t dataOffset;	/* The offset, in bytes, from the beginning
							of the BITMAPFILEHEADER structure to the bitmap bits. */
	uint32_t infoHeaderSize;/* The number of bytes required by the structure.
							This value does not include the size of the color table
							or the size of the color masks. */
	int32_t  width;         /* The width of the bitmap, in pixels. */
	int32_t  height;        /* The height of the bitmap, in pixels. */
	uint16_t bitCount;      /* The number of bits per pixel (bpp). */
	uint32_t compression;   /* The type of compression used. */
	uint32_t colorUsed;		/* The number of colors used. */
	uint32_t lineSize;		/* The size of a bitmap line (in bytes). */
	uint8_t  header[54];
	uint8_t* pHeader;
	uint32_t bytesRead;
	uint8_t* outData;
	uint32_t paletteSize;
	uint8_t* lineData;
	uint16_t* outPixel;
	uint32_t line;
	uint8_t* ptr;
	uint32_t rMask;
	uint32_t gMask;
	uint32_t bMask;

	if (!img || !fp)
		return stm32ipl_err_InvalidParameter;

	image_init(img, 0, 0, 0, 0);

	if (f_lseek(fp, 0) != FR_OK)
		return stm32ipl_err_SeekingFile;

	/* Read file header and info header (54 bytes). */
	if ((f_read(fp, header, sizeof(header), (UINT*)&bytesRead) != FR_OK) || bytesRead != sizeof(header))
		return stm32ipl_err_ReadingFile;

	pHeader = (uint8_t*)header;

	/* Read the data offset. */
	dataOffset = pHeader[10] + (pHeader[11] << 8) + (pHeader[12] << 16) + (pHeader[13] << 24);

	/* Read the size of the info header. */
	infoHeaderSize = pHeader[14] + (pHeader[15] << 8) + (pHeader[16] << 16) + (pHeader[17] << 24);
	if ((infoHeaderSize != 40)				/* BITMAPINFOHEADER */
			&& (infoHeaderSize != 52)		/* BITMAPV2INFOHEADER */
			&& (infoHeaderSize != 56)		/* BITMAPV3INFOHEADER */
			&& (infoHeaderSize != 108)		/* BITMAPV4HEADER */
			&& (infoHeaderSize != 124)) 	/* BITMAPV5HEADER */
		return stm32ipl_err_UnsupportedFormat;

	/* Read the bitmap width. */
	width = pHeader[18] + (pHeader[19] << 8) + (pHeader[20] << 16) + (pHeader[21] << 24);

	/* Read the bitmap height. */
	height = pHeader[22] + (pHeader[23] << 8) + (pHeader[24] << 16) + (pHeader[25] << 24);

	/* Read the bpp. */
	bitCount = pHeader[28] + (pHeader[29] << 8);
	if ((bitCount != 1) && (bitCount != 4) && (bitCount != 8) && (bitCount != 16) && (bitCount != 24))
		return stm32ipl_err_UnsupportedFormat;

	/* Read the compression. Only BI_RGB and BI_BITFIELDS are supported. */
	compression = pHeader[30] + (pHeader[31] << 8) + (pHeader[32] << 16)  + (pHeader[33] << 24);
	if (compression != BI_RGB && compression != BI_BITFIELDS)
		return stm32ipl_err_UnsupportedFormat;

	/* Read the number of colors used in the palette. */
	colorUsed = pHeader[46] + (pHeader[47] << 8) + (pHeader[48] << 16)  + (pHeader[49] << 24);
	if (colorUsed == 0)
		colorUsed = 1 << bitCount;

	lineSize = (((width * bitCount) + 31) / 32) * 4;

	/* Evaluate eventual bit fields. */
	if ((compression == BI_BITFIELDS) && (bitCount == 16)) {
		uint8_t mask[4];
		/* Read the three bit masks. */
		if ((f_read(fp, mask, sizeof(mask), (UINT*)&bytesRead) != FR_OK) || bytesRead != sizeof(mask))
			return stm32ipl_err_ReadingFile;

		rMask = mask[0] + (mask[1] << 8) + (mask[2] << 16) + (mask[3] << 24);

		if ((f_read(fp, mask, sizeof(mask), (UINT*)&bytesRead) != FR_OK) || bytesRead != sizeof(mask))
			return stm32ipl_err_ReadingFile;

		gMask = mask[0] + (mask[1] << 8) + (mask[2] << 16) + (mask[3] << 24);

		if ((f_read(fp, mask, sizeof(mask), (UINT*)&bytesRead) != FR_OK) || bytesRead != sizeof(mask))
			return stm32ipl_err_ReadingFile;

		bMask = mask[0] + (mask[1] << 8) + (mask[2] << 16) + (mask[3] << 24);
	} else {
		/* Default for no compression is RGB555. */
		rMask = RGB555_RED_MASK;
		gMask = RGB555_GREEN_MASK;
		bMask = RGB555_BLUE_MASK;
	}

	switch (bitCount) {
	case 1: {
		uint32_t palette[2];

		paletteSize = colorUsed * sizeof(uint32_t);

		/* Skip the header. */
		if (f_lseek(fp, dataOffset - paletteSize) != FR_OK)
			return stm32ipl_err_SeekingFile;

		/* Read the palette. */
		if ((f_read(fp, palette, paletteSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != paletteSize)
			return stm32ipl_err_ReadingFile;

		/* Allocate memory for pixel data (RGB565). */
		outData = xalloc(width * abs(height) * 2);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		lineData = xalloc(lineSize);
		if (!lineData) {
			xfree(outData);
			return stm32ipl_err_OutOfMemory;
		}

		/* Jump to the first or last line. */
		line = dataOffset + ((height > 0) ? (lineSize * (height - 1)) : 0);
		if (f_lseek(fp, line) != FR_OK) {
			xfree(lineData);
			xfree(outData);
			return stm32ipl_err_SeekingFile;
		}

		outPixel = (uint16_t*)outData;
		for (uint32_t i = 0; i < abs(height); i++) {
			uint8_t value = 0;
			uint8_t k = 0;

			if ((f_read(fp, lineData, lineSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != lineSize) {
				xfree(lineData);
				xfree(outData);
				return stm32ipl_err_ReadingFile;
			}

			ptr = lineData;

			for (uint32_t j = 0; j < width; k--, j++) {
				uint8_t index;
				uint8_t b;
				uint8_t g;
				uint8_t r;

				if (!(j % 8)) {
					value = (*ptr++);
					k = 7;
				}

				index = (value >> k) & 0x1;
				b = palette[index];
				g = palette[index] >> 8;
				r = palette[index] >> 16;
				*outPixel++ = (uint16_t)COLOR_R8_G8_B8_TO_RGB565(r, g, b);
			}

			if (height > 0) {
				line -= lineSize;
				if (line > dataOffset) {
					if (f_lseek(fp, line) != FR_OK) {
						xfree(lineData);
						xfree(outData);
						return stm32ipl_err_SeekingFile;
					}
				}
			} else {
				line += lineSize;
			}
		}

		xfree(lineData);
		lineData = 0;

		image_init(img, width, abs(height), IMAGE_BPP_RGB565, outData);

		break;
	}

	case 4: {
		uint32_t palette[16];

		paletteSize = colorUsed * sizeof(uint32_t);

		/* Skip the header. */
		if (f_lseek(fp, dataOffset - paletteSize) != FR_OK)
			return stm32ipl_err_SeekingFile;

		/* Read the palette. */
		if ((f_read(fp, palette, paletteSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != paletteSize)
			return stm32ipl_err_ReadingFile;

		/* Allocate memory for pixel data (RGB565). */
		outData = xalloc(width * abs(height) * 2);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		lineData = xalloc(lineSize);
		if (!lineData) {
			xfree(outData);
			return stm32ipl_err_OutOfMemory;
		}

		/* Jump to the first or last line. */
		line = dataOffset + ((height > 0) ? (lineSize * (height - 1)) : 0);
		if (f_lseek(fp, line) != FR_OK) {
			xfree(lineData);
			xfree(outData);
			return stm32ipl_err_SeekingFile;
		}

		outPixel = (uint16_t*)outData;
		for (uint32_t i = 0; i < abs(height); i++) {
			if ((f_read(fp, lineData, lineSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != lineSize) {
				xfree(lineData);
				xfree(outData);
				return stm32ipl_err_ReadingFile;
			}

			ptr = lineData;
			for (uint32_t j = 0; j < width;) {
				uint8_t value = (*ptr++);
				uint8_t index = value >> 4;
				uint8_t b = palette[index];
				uint8_t g = palette[index] >> 8;
				uint8_t r = palette[index] >> 16;
				*outPixel++ = (uint16_t)COLOR_R8_G8_B8_TO_RGB565(r, g, b);

				j++;
				if (j < width) {
					index = value & 0xF;
					b = palette[index];
					g = palette[index] >> 8;
					r = palette[index] >> 16;
					*outPixel++ = (uint16_t)COLOR_R8_G8_B8_TO_RGB565(r, g, b);
				}
				j++;
			}

			if (height > 0) {
				line -= lineSize;
				if (line > dataOffset) {
					if (f_lseek(fp, line) != FR_OK) {
						xfree(lineData);
						xfree(outData);
						return stm32ipl_err_SeekingFile;
					}
				}
			} else {
				line += lineSize;
			}
		}

		xfree(lineData);
		lineData = 0;

		image_init(img, width, abs(height), IMAGE_BPP_RGB565, outData);

		break;
	}

	case 8:  {
		uint32_t palette[256];

		paletteSize = colorUsed * sizeof(uint32_t);

		/* Skip the header. */
		if (f_lseek(fp, dataOffset - paletteSize) != FR_OK)
			return stm32ipl_err_SeekingFile;

		/* Read the palette. */
		if ((f_read(fp, palette, paletteSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != paletteSize)
			return stm32ipl_err_ReadingFile;

		/* Allocate memory for pixel data (RGB565). */
		outData = xalloc(width * abs(height) * 2);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		lineData = xalloc(lineSize);
		if (!lineData) {
			xfree(outData);
			return stm32ipl_err_OutOfMemory;
		}

		/* Jump to the first or last line. */
		line = dataOffset + ((height > 0) ? (lineSize * (height - 1)) : 0);
		if (f_lseek(fp, line) != FR_OK) {
			xfree(lineData);
			xfree(outData);
			return stm32ipl_err_SeekingFile;
		}

		outPixel = (uint16_t*)outData;
		for (uint32_t i = 0; i < abs(height); i++) {
			if ((f_read(fp, lineData, lineSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != lineSize) {
				xfree(lineData);
				xfree(outData);
				return stm32ipl_err_ReadingFile;
			}

			ptr = lineData;
			for (uint32_t j = 0; j < width; j++) {
				uint8_t index = (*ptr++);
				uint8_t b = palette[index];
				uint8_t g = palette[index] >> 8;
				uint8_t r = palette[index] >> 16;
				*outPixel++ = (uint16_t)COLOR_R8_G8_B8_TO_RGB565(r, g, b);
			}

			if (height > 0) {
				line -= lineSize;
				if (line > dataOffset) {
					if (f_lseek(fp, line) != FR_OK) {
						xfree(lineData);
						xfree(outData);
						return stm32ipl_err_SeekingFile;
					}
				}
			} else {
				line += lineSize;
			}
		}

		xfree(lineData);
		lineData = 0;

		image_init(img, width, abs(height), IMAGE_BPP_RGB565, outData);

		break;
	}

	case 16: {
		uint16_t *inPixel;

		/* Skip the header. */
		if (f_lseek(fp, dataOffset) != FR_OK)
			return stm32ipl_err_SeekingFile;

		/* Allocate memory for pixel data (RGB565). */
		outData = xalloc(width * abs(height) * 2);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		lineData = xalloc(lineSize);
		if (!lineData) {
			xfree(outData);
			return stm32ipl_err_OutOfMemory;
		}

		/* Jump to the first or last line. */
		line = dataOffset + ((height > 0) ? (lineSize * (height - 1)) : 0);

		if (f_lseek(fp, line) != FR_OK) {
			xfree(outData);
			return stm32ipl_err_SeekingFile;
		}

		outPixel = (uint16_t*)outData;

		for (uint32_t i = 0; i < abs(height); i++) {
			if ((f_read(fp, lineData, lineSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != lineSize) {
				xfree(lineData);
				xfree(outData);
				return stm32ipl_err_ReadingFile;
			}

			inPixel = (uint16_t*)lineData;

			for (uint32_t j = 0; j < width; j++) {
				uint16_t value = *inPixel;

				if (compression && (rMask == RGB565_RED_MASK) && (gMask == RGB565_GREEN_MASK) && (bMask == RGB565_BLUE_MASK))
					/* RGB555 case. */
					*outPixel++ = value;
				else
					/* RGB555 case. */
					*outPixel++ = ((value & rMask) << 1) | ((value & gMask) << 1) | (value & bMask);

				inPixel++;
			}

			if (height > 0) {
				line -= lineSize;
				if (line > dataOffset) {
					if (f_lseek(fp, line) != FR_OK) {
						xfree(lineData);
						xfree(outData);
						return stm32ipl_err_SeekingFile;
					}
				}
			} else {
				line += lineSize;
			}
		}

		xfree(lineData);
		lineData = 0;

		image_init(img, width, abs(height), IMAGE_BPP_RGB565, outData);

		break;
	}

	case 24: {
		/* Skip the header. */
		if (f_lseek(fp, dataOffset) != FR_OK)
			return stm32ipl_err_SeekingFile;

		/* Allocate memory for pixel data (RGB565). */
		outData = xalloc(width * abs(height) * 2);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		lineData = xalloc(lineSize);
		if (!lineData) {
			xfree(outData);
			return stm32ipl_err_OutOfMemory;
		}

		/* Jump to the first or last line. */
		line = dataOffset + ((height > 0) ? (lineSize * (height - 1)) : 0);

		if (f_lseek(fp, line) != FR_OK) {
			xfree(lineData);
			xfree(outData);
			return stm32ipl_err_SeekingFile;
		}

		outPixel = (uint16_t*)outData;

		for (uint32_t i = 0; i < abs(height); i++) {
			if ((f_read(fp, lineData, lineSize, (UINT*)&bytesRead) != FR_OK) || bytesRead != lineSize) {
				xfree(lineData);
				xfree(outData);
				return stm32ipl_err_ReadingFile;
			}

			ptr = lineData;
			for (uint32_t j = 0; j < width; j++) {
				uint8_t b = *ptr++;
				uint8_t g = *ptr++;
				uint8_t r = *ptr++;
				*outPixel++ = (uint16_t)COLOR_R8_G8_B8_TO_RGB565(r, g, b);
			}

			if (height > 0) {
				line -= lineSize;
				if (line > dataOffset) {
					if (f_lseek(fp, line) != FR_OK) {
						xfree(lineData);
						xfree(outData);
						return stm32ipl_err_SeekingFile;
					}
				}
			} else {
				line += lineSize;
			}
		}

		xfree(lineData);
		lineData = 0;

		image_init(img, width, abs(height), IMAGE_BPP_RGB565, outData);

		break;
	}

	default:
		return stm32ipl_err_UnsupportedFormat;
	};

	return stm32ipl_err_Ok;
}


/* Read PNM image file.
 * img: The image read. The pixel data is allocated internally
 * and must be released with STM32Ipl_ReleaseData(); assuming that input
 * img->data is null.
 * fp Pointer to the input file structure.
 * return stm32ipl_err_Ok on success, errors otherwise
 */
static stm32ipl_err_t read_pnm(image_t *img, FIL *fp)
{
	uint32_t size = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	UINT bytesRead = 0;
	uint32_t number = 0;
	uint8_t sector[512];
	uint8_t number_ppm = 0;
	bool valid = false;
	uint8_t* outData;

	enum { EAT_WHITESPACE, EAT_COMMENT, EAT_NUMBER } mode = EAT_WHITESPACE;

	if (!img || !fp)
		return stm32ipl_err_InvalidParameter;

	image_init(img, 0, 0, 0, 0);

	if (f_lseek(fp, 0) != FR_OK)
		return stm32ipl_err_SeekingFile;

	if ((f_read(fp, sector, 2, (UINT *)&bytesRead) != FR_OK) || bytesRead != 2)
		return stm32ipl_err_ReadingFile;

	/* Read bpp. */
	number_ppm = sector[1];

	if ((number_ppm != '2') && (number_ppm != '3') && (number_ppm != '5') && (number_ppm != '6'))
		return stm32ipl_err_UnsupportedFormat;

	do {
		if ((f_read(fp, sector, 1, &bytesRead) != FR_OK) || bytesRead != 1)
			return stm32ipl_err_ReadingFile;

		if (mode == EAT_WHITESPACE) {
			if (sector[0] == '#') {
				mode = EAT_COMMENT;
			} else
			if (('0' <= sector[0]) && (sector[0] <= '9')) {
				number = sector[0] - '0';
				mode = EAT_NUMBER;
			}
		} else
		if (mode == EAT_COMMENT) {
			if ((sector[0] == '\n') || (sector[0] == '\r')) {
				mode = EAT_WHITESPACE;
			}
		} else
		if (mode == EAT_NUMBER) {
			if (('0' <= sector[0]) && (sector[0] <= '9')) {
				number = (number * 10) + sector[0] - '0';
			} else {
				valid = true;
			}
		}
	} while (!valid);

	width  = number;
	number = 0;
	mode   = EAT_WHITESPACE;

	do {
		if (valid) {
			valid = false;
		} else {
			if ((f_read(fp, sector, 1, &bytesRead) != FR_OK) || bytesRead != 1)
				return stm32ipl_err_ReadingFile;
		}

		if ( mode == EAT_WHITESPACE) {
			if (sector[0] == '#') {
				mode = EAT_COMMENT;
			} else
			if (('0' <= sector[0]) && (sector[0] <= '9')) {
				number = sector[0] - '0';
				mode = EAT_NUMBER;
			}
		} else
		if (mode == EAT_COMMENT) {
			if ((sector[0] == '\n') || (sector[0] == '\r')) {
				mode = EAT_WHITESPACE;
			}
		} else
		if (mode == EAT_NUMBER) {
			if (('0' <= sector[0]) && (sector[0] <= '9')) {
				number = (number * 10 ) + sector[0] - '0';
			} else {
				valid = true;
			}
		}
	} while (!valid);

	height = number;
	number = 0;
	mode   = EAT_WHITESPACE;

	if (height == 0 || width == 0)
		return stm32ipl_err_InvalidParameter;

	do {
		if (valid) {
			valid = false;
		} else {
		if ((f_read(fp, sector, 1, &bytesRead) != FR_OK) || bytesRead != 1)
			return stm32ipl_err_ReadingFile;
		}

		if (mode == EAT_WHITESPACE) {
			if (sector[0] == '#') {
				mode = EAT_COMMENT;
			} else
			if (('0' <= sector[0]) && (sector[0] <= '9')) {
				number = sector[0] - '0';
				mode = EAT_NUMBER;
			}
		} else
		if (mode == EAT_COMMENT) {
			if ((sector[0] == '\n') || (sector[0] == '\r')) {
				mode = EAT_WHITESPACE;
			}
		} else
		if (mode == EAT_NUMBER) {
			if (('0' <= sector[0]) && (sector[0] <= '9')) {
				number = (number * 10 ) + sector[0] - '0';
			} else {
				valid = true;
			}
		}
	} while (!valid);

	if (number > 255)
		return stm32ipl_err_Generic;

	switch (number_ppm) {
	case '2': {
		outData = xalloc(width * height);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		uint8_t *pixel_luma_dst = (uint8_t *)outData;

		for (uint32_t i = 0; i < height; i++) {
			uint32_t offset = i * width;

			for (uint32_t j = 0; j < width; j++) {
				number = 0;
				mode   = EAT_WHITESPACE;

				do {
					if (valid) {
						valid = false;
					} else {
						if ((f_read(fp, sector, 1, &bytesRead) != FR_OK) || bytesRead != 1)
							return stm32ipl_err_ReadingFile;
					}
					if (mode == EAT_WHITESPACE ) {
						if (sector[0] == '#') {
							mode = EAT_COMMENT;
						} else
						if (('0' <= sector[0]) && (sector[0] <= '9')) {
							number = sector[0] - '0';
							mode = EAT_NUMBER;
						}
					} else
					if (mode == EAT_COMMENT) {
						if ((sector[0] == '\n') || (sector[0] == '\r')) {
							mode = EAT_WHITESPACE;
						}
					} else
					if (mode == EAT_NUMBER) {
						if (('0' <= sector[0]) && (sector[0] <= '9')) {
							number = (number * 10 ) + sector[0] - '0';
						} else {
							valid = true;
						}
					}
				} while (!valid);

				pixel_luma_dst[offset + j] = (uint8_t)number;
			}
		}

		image_init(img, width, height, IMAGE_BPP_GRAYSCALE, outData);

		break;
	}

	case '3': {
		outData = xalloc(width * height * 2);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		uint16_t *pixel_rgb16 = (uint16_t *)outData;
		uint16_t r = 0;
		uint16_t g = 0;
		uint16_t b = 0;

		for (uint32_t i = 0; i < height; i++) {
			uint32_t offset = i * width;

			for (uint32_t j = 0; j < width; j++) {
				for (uint8_t rgb = 0; rgb < 3; rgb++) {
					number = 0;
					mode   = EAT_WHITESPACE;

					do {
						if (valid) {
							valid = false;
						} else {
							if ((f_read(fp, sector, 1, &bytesRead) != FR_OK) || bytesRead != 1)
								return stm32ipl_err_ReadingFile;
						}

						if (mode == EAT_WHITESPACE) {
							if (sector[0]=='#') {
								mode = EAT_COMMENT;
							} else
							if (('0' <= sector[0]) && (sector[0] <= '9')) {
								number = sector[0] - '0';
								mode = EAT_NUMBER;
							}
						} else
						if (mode == EAT_COMMENT) {
							if ((sector[0] == '\n') || (sector[0] == '\r')) {
								mode = EAT_WHITESPACE;
							}
						} else
						if (mode == EAT_NUMBER) {
							if (('0' <= sector[0]) && (sector[0] <= '9')) {
								number = (number * 10) + sector[0] - '0';
							} else {
								valid = true;
							}
						}
					} while (!valid);

					if (rgb == 0) {
						r = number;
					} else
					if (rgb == 1) {
						g = number;
					} else
					if (rgb == 2) {
						b = number;
						pixel_rgb16[offset + j] = COLOR_R8_G8_B8_TO_RGB565(r, g, b);
					}
				}
			}
		}

		image_init(img, width, height, IMAGE_BPP_RGB565, outData);

		break;
	}

	case '5': {
		size = width * height;
		outData = xalloc(size);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		if ((f_read(fp, outData, size, &bytesRead) != FR_OK) || bytesRead != size)
			return stm32ipl_err_ReadingFile;

		image_init(img, width, height, IMAGE_BPP_GRAYSCALE, outData);

		break;
	}

	case '6': {
		outData = xalloc(width * height * 2);
		if (!outData)
			return stm32ipl_err_OutOfMemory;

		uint16_t* pixel_rgb16 = (uint16_t*)outData;

		for (uint32_t i = 0; i < height; i++) {
			uint32_t offset = i * width;
			for (uint32_t j = 0; j < width; j++) {
				if ((f_read(fp, sector, 3, &bytesRead) != FR_OK) || bytesRead != 3)
					return stm32ipl_err_ReadingFile;

				pixel_rgb16[offset + j] = COLOR_R8_G8_B8_TO_RGB565(sector[0], sector[1], sector[2]);
			}
		}

		image_init(img, width, height, IMAGE_BPP_RGB565, outData);

		break;
	}

	default:
		return stm32ipl_err_InvalidParameter;
	}

	return stm32ipl_err_Ok;
}


/* Read JPEG image file.
 * img: The image read. The pixel data is allocated internally
 * and must be released with STM32Ipl_ReleaseData(); assuming that input
 * img->data is null.
 * fp Pointer to the input file structure.
 * return stm32ipl_err_Ok on success, errors otherwise
 */
static stm32ipl_err_t read_jpg(image_t *img, FIL* fp)
{
#ifdef STM32IPL_USE_HW_JPEG_CODEC
	return readJPEGHW(img, fp);
#else
	return readJPEGSW(img, fp);
#endif
}


/**
 * @brief Write the given image to file; the target file format is determined
 * by the filename extension; the supported formats are: BMP, PNM, JPG
 * @param img		The image to be saved
 * @param filename 	The name of the output file
 * @return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t STM32Ipl_WriteImage(const image_t *img, const char *filename)
{
	if (!img || !img->data | !filename)
		return stm32ipl_err_InvalidParameter;

	if (img->bpp != IMAGE_BPP_GRAYSCALE && img->bpp != IMAGE_BPP_RGB565)
		return stm32ipl_err_UnsupportedFormat;

	switch (get_image_file_format(filename)){
	case iplFileFormatBMP:
		return save_bmp(img, filename);

	case iplFileFormatPNM:
		return save_pnm(img, filename, false);

	case iplFileFormatJPG:
		return save_jpg(img, filename);

	default:
		break;
	}

	return stm32ipl_err_UnsupportedFormat;
}


/* Swap B and R channels within a RGB888 line.
 * line: pointer to the image line.
 * width: width of the image.
 */
static void swapRGB888Channels(uint8_t *line, uint32_t width)
{
	RGB888_t *rgb = (RGB888_t*)line;

	for (uint32_t i = 0; i < width; i++) {
		uint8_t tmp = rgb[i].b;
		rgb[i].b = rgb[i].r;
		rgb[i].r = tmp;
	}
}


/* Save image to BMP file.
 * img: the image to be saved
 * filename: The name of the output file
 * return stm32ipl_err_Ok on success, errors otherwise
 */
static stm32ipl_err_t save_bmp(const image_t *img, const char *filename)
{
	FIL fp;
	FRESULT res;
	uint32_t size = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bit_pixel = 0;
	uint32_t row_bytes = 0;
	uint32_t data_size = 0;

	uint32_t fix_size = 0;
	UINT byteswritten;

	uint16_t data16;
	uint32_t data32;
	uint32_t padding;

	uint8_t bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	uint8_t bmpinfoheader[40] = { 0 };

	if (!img || !filename)
		return stm32ipl_err_InvalidParameter;

	bit_pixel = img->bpp * 8;
	width     = img->w;
	height    = img->h;

	if (f_open(&fp, (const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return stm32ipl_err_OpeningFile;

	switch (bit_pixel) {
	case 8: {
		fix_size  = 14 + 40 + 1024;
		row_bytes = (((width * 8) + 31) / 32) * 4;
		data_size = (row_bytes * height);
		size      = fix_size + data_size;
		padding   = (row_bytes / sizeof(uint8_t)) - width;

		bmpfileheader[5]  = (uint8_t)((size >> 24) & 0xFF);
		bmpfileheader[4]  = (uint8_t)((size >> 16) & 0xFF);
		bmpfileheader[3]  = (uint8_t)((size >> 8) & 0XFF);
		bmpfileheader[2]  = (uint8_t)((size & 0XFF));
		/* Byte 6, 7, 8, 9 equal to 0 */
		bmpfileheader[13] = (uint8_t)((fix_size >> 24) & 0xFF);
		bmpfileheader[12] = (uint8_t)((fix_size >> 16) & 0xFF);
		bmpfileheader[11] = (uint8_t)((fix_size >> 8) & 0XFF);
		bmpfileheader[10] = (uint8_t)((fix_size & 0XFF));

		/* Info header (40 bytes) */
		data32 = 40;
		bmpinfoheader[3] = (uint8_t)((data32 >> 24) & 0xFF);
		bmpinfoheader[2] = (uint8_t)((data32 >> 16) & 0xFF);
		bmpinfoheader[1] = (uint8_t)((data32 >> 8) & 0XFF);
		bmpinfoheader[0] = (uint8_t)((data32 & 0XFF));

		bmpinfoheader[7] = (uint8_t)((width >> 24) & 0xFF);
		bmpinfoheader[6] = (uint8_t)((width >> 16) & 0xFF);
		bmpinfoheader[5] = (uint8_t)((width >> 8) & 0xFF);
		bmpinfoheader[4] = (uint8_t)((width & 0XFF));

		bmpinfoheader[11] = (uint8_t)((height >> 24) & 0xFF);
		bmpinfoheader[10] = (uint8_t)((height >> 16) & 0xFF);
		bmpinfoheader[9]  = (uint8_t)((height >> 8) & 0xFF);
		bmpinfoheader[8]  = (uint8_t)((height & 0XFF));

		data16 = 1;
		bmpinfoheader[13] = (uint8_t)((data16 >> 8) & 0xFF);
		bmpinfoheader[12] = (uint8_t)((data16 & 0xFF));
		data16 = 8;
		bmpinfoheader[15] = (uint8_t)((data16 >> 8) & 0xFF);
		bmpinfoheader[14] = (uint8_t)((data16 & 0XFF));

		bmpinfoheader[23] = (uint8_t)((data_size >> 24) & 0xFF);
		bmpinfoheader[22] = (uint8_t)((data_size >> 16) & 0xFF);
		bmpinfoheader[21] = (uint8_t)((data_size >> 8) & 0xFF);
		bmpinfoheader[20] = (uint8_t)((data_size & 0XFF));

		/* Write header */
		res = f_write(&fp, (uint8_t *)bmpfileheader, sizeof(bmpfileheader), &byteswritten);
		if (res != FR_OK || byteswritten != 14) {
			f_close(&fp);
			return stm32ipl_err_WritingFile;
		}

		res = f_write(&fp, (uint8_t *)bmpinfoheader, sizeof(bmpinfoheader), &byteswritten);
		if (res != FR_OK || byteswritten != 40){
			f_close(&fp);
			return stm32ipl_err_WritingFile;
		}

		/* Color table. */
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t data = ((i) << 16) | ((i) << 8) | i;
			res = f_write(&fp, &data, sizeof(data), &byteswritten);
			if (res != FR_OK || byteswritten != 4) {
				f_close(&fp);
				return stm32ipl_err_WritingFile;
			}
		}

		if (width == row_bytes) {
			for (uint32_t i = 0; i < height; i++ ) {
				uint32_t scanline = (height - i - 1) * width;
				res = f_write(&fp, img->data + scanline, width, &byteswritten);
				if (res != FR_OK || byteswritten != width) {
					f_close(&fp);
					return stm32ipl_err_WritingFile;
				}
			}
		} else {
			for (uint32_t i = 0; i < height; i++){
				uint32_t scanline = (height - i - 1) * width;
				res = f_write(&fp, img->data + scanline, width, &byteswritten);
				if (res != FR_OK || byteswritten != width) {
					f_close(&fp);
					return stm32ipl_err_WritingFile;
				}

				for (uint32_t j = 0; j < padding; j++) {
					uint8_t zero = 0;
					res = f_write(&fp, &zero, 1, &byteswritten);
					if (res != FR_OK || byteswritten != 1) {
						f_close(&fp);
						return stm32ipl_err_WritingFile;
					}
				}
			}
		}
		break;
	}

	case 16: {
		uint16_t * dataToWrite = (uint16_t *)img->data;

		fix_size = 14 + 40 + 12;
		row_bytes = (((width * 16) + 31) / 32) * 4;
		data_size = (row_bytes * height);
		size      = fix_size + data_size;
		padding   = (row_bytes / sizeof(uint16_t)) - width;

		bmpfileheader[5] = (uint8_t)((size >> 24) & 0xFF);
		bmpfileheader[4] = (uint8_t)((size >> 16) & 0xFF);
		bmpfileheader[3] = (uint8_t)((size >> 8) & 0XFF);
		bmpfileheader[2] = (uint8_t)((size & 0XFF));

		/* Byte 6, 7, 8, 9 equal to 0 */

		bmpfileheader[13] = (uint8_t)((fix_size >> 24) & 0xFF);
		bmpfileheader[12] = (uint8_t)((fix_size >> 16) & 0xFF);
		bmpfileheader[11] = (uint8_t)((fix_size >> 8) & 0XFF);
		bmpfileheader[10] = (uint8_t)((fix_size & 0XFF));

		/* Info header (40 bytes) */
		data32 = 40;
		bmpinfoheader[3] = (uint8_t)((data32 >> 24) & 0xFF);
		bmpinfoheader[2] = (uint8_t)((data32 >> 16) & 0xFF);
		bmpinfoheader[1] = (uint8_t)((data32 >> 8) & 0XFF);
		bmpinfoheader[0] = (uint8_t)((data32 & 0XFF));

		bmpinfoheader[7] = (uint8_t)((width >> 24) & 0xFF);
		bmpinfoheader[6] = (uint8_t)((width >> 16) & 0xFF);
		bmpinfoheader[5] = (uint8_t)((width >> 8) & 0xFF);
		bmpinfoheader[4] = (uint8_t)((width & 0XFF));

		bmpinfoheader[11] = (uint8_t)((height >> 24) & 0xFF);
		bmpinfoheader[10] = (uint8_t)((height >> 16) & 0xFF);
		bmpinfoheader[9] = (uint8_t)((height >> 8) & 0xFF);
		bmpinfoheader[8] = (uint8_t)((height & 0XFF));

		data16 = 1;
		bmpinfoheader[13] = (uint8_t)((data16 >> 8) & 0xFF);
		bmpinfoheader[12] = (uint8_t)((data16 & 0xFF));
		data16 = 16;
		bmpinfoheader[15] = (uint8_t)((data16 >> 8) & 0xFF);
		bmpinfoheader[14] = (uint8_t)((data16 & 0XFF));
		data32 = 3;
		bmpinfoheader[19] = (uint8_t)((data32 >> 24) & 0xFF);
		bmpinfoheader[18] = (uint8_t)((data32 >> 16) & 0xFF);
		bmpinfoheader[17] = (uint8_t)((data32 >> 8) & 0xFF);
		bmpinfoheader[16] = (uint8_t)((data32 & 0XFF));

		bmpinfoheader[23] = (uint8_t)((data_size >> 24) & 0xFF);
		bmpinfoheader[22] = (uint8_t)((data_size >> 16) & 0xFF);
		bmpinfoheader[21] = (uint8_t)((data_size >> 8) & 0xFF);
		bmpinfoheader[20] = (uint8_t)((data_size & 0XFF));

		/* Write header */
		res = f_write(&fp, (uint8_t *)bmpfileheader, sizeof(bmpfileheader), &byteswritten);
		if (res != FR_OK || byteswritten != 14) {
			f_close(&fp);
			return stm32ipl_err_WritingFile;
		}

		res = f_write(&fp, (uint8_t *)bmpinfoheader, sizeof(bmpinfoheader), &byteswritten);
		if (res != FR_OK || byteswritten != 40) {
			f_close(&fp);
			return stm32ipl_err_WritingFile;
		}

		/* Bit masks (12 bytes) */
		data32 = (uint32_t)0x1F << 11;
		res = f_write(&fp, (uint8_t*)&data32, sizeof(uint32_t), &byteswritten);
		data32 = (uint32_t)0x3F << 5;
		res = f_write(&fp, (uint8_t*)&data32, sizeof(uint32_t), &byteswritten);
		data32 = (uint32_t)0x1F ;
		res = f_write(&fp, (uint8_t*)&data32, sizeof(uint32_t), &byteswritten);

		if (width * 2 == row_bytes) {
			for (uint32_t i = 0; i < height; i++ ) {
					uint32_t scanline = (height - i) * width - (width);
					for (uint32_t j = 0; j < width; j++) {
						uint16_t pixel565 = (dataToWrite + scanline)[j];
						res = f_write(&fp, &pixel565, 2, &byteswritten);
						if (res != FR_OK || byteswritten != 2) {
							f_close(&fp);
							return stm32ipl_err_WritingFile;
						}
					}
			}
		} else {
			for (uint32_t i = 0; i < height; i++) {
				uint32_t scanline = (height - i) * width - (width);
				for (uint32_t j = 0; j < width; j++) {
					uint16_t pixel565 = (dataToWrite + scanline)[j];
					res = f_write(&fp, &pixel565, 2, &byteswritten);
					if (res != FR_OK || byteswritten != 2) {
						f_close(&fp);
						return stm32ipl_err_WritingFile;
					}
				}
				for (uint32_t j = 0; j < padding; j++) {
					uint16_t zero = 0;
					res = f_write(&fp, &zero, sizeof(uint16_t), &byteswritten);
					if (res != FR_OK || byteswritten != padding * 2) {
						f_close(&fp);
						return stm32ipl_err_WritingFile;
					}
				}
			}
		}
		break;
	}

	case 24: {
		RGB888_t *dataToWrite = (RGB888_t*)img->data;

		fix_size = 14 + 40;
		row_bytes   = (((width * 24) + 31) / 32) * 4;
		data_size   = (row_bytes * height);
		size        = fix_size + data_size;
		padding     = row_bytes - (width * 3);

		bmpfileheader[5] = (uint8_t)((size >> 24) & 0xFF) ;
		bmpfileheader[4] = (uint8_t)((size >> 16) & 0xFF) ;
		bmpfileheader[3] = (uint8_t)((size >> 8) & 0XFF);
		bmpfileheader[2] = (uint8_t)((size & 0XFF));

		/* Byte 6, 7, 8, 9 equal to 0 */
		bmpfileheader[13] = (uint8_t)((fix_size >> 24) & 0xFF);
		bmpfileheader[12] = (uint8_t)((fix_size >> 16) & 0xFF);
		bmpfileheader[11] = (uint8_t)((fix_size >> 8) & 0XFF);
		bmpfileheader[10] = (uint8_t)((fix_size & 0XFF));

		/* Info header (40 bytes) */
		data32 = 40;
		bmpinfoheader[3] = (uint8_t)((data32 >> 24) & 0xFF);
		bmpinfoheader[2] = (uint8_t)((data32 >> 16) & 0xFF);
		bmpinfoheader[1] = (uint8_t)((data32 >> 8) & 0XFF);
		bmpinfoheader[0] = (uint8_t)((data32 & 0XFF));

		bmpinfoheader[7] = (uint8_t)((width >> 24) & 0xFF);
		bmpinfoheader[6] = (uint8_t)((width >> 16) & 0xFF);
		bmpinfoheader[5] = (uint8_t)((width >> 8) & 0xFF);
		bmpinfoheader[4] = (uint8_t)((width & 0XFF));

		bmpinfoheader[11] = (uint8_t)((height >> 24) & 0xFF);
		bmpinfoheader[10] = (uint8_t)((height >> 16) & 0xFF);
		bmpinfoheader[9] = (uint8_t)((height >> 8) & 0xFF);
		bmpinfoheader[8] = (uint8_t)((height & 0XFF));

		data16 = 1;
		bmpinfoheader[13] = (uint8_t)((data16 >> 8) & 0xFF);
		bmpinfoheader[12] = (uint8_t)((data16 & 0xFF));
		data16 = 24;
		bmpinfoheader[15] = (uint8_t)((data16 >> 8) & 0xFF);
		bmpinfoheader[14] = (uint8_t)((data16 & 0XFF));

		data32 = 0;
		bmpinfoheader[19] = (uint8_t)((data32 >> 24) & 0xFF);
		bmpinfoheader[18] = (uint8_t)((data32 >> 16) & 0xFF);
		bmpinfoheader[17] = (uint8_t)((data32 >> 8) & 0xFF);
		bmpinfoheader[16] = (uint8_t)((data32 & 0XFF));

		bmpinfoheader[23] = (uint8_t)((data_size >> 24) & 0xFF);
		bmpinfoheader[22] = (uint8_t)((data_size >> 16) & 0xFF);
		bmpinfoheader[21] = (uint8_t)((data_size >> 8) & 0xFF);
		bmpinfoheader[20] = (uint8_t)((data_size & 0XFF));

		/* Write header */
		res = f_write(&fp, (uint8_t *)bmpfileheader, sizeof(bmpfileheader), &byteswritten);
		if (res != FR_OK || byteswritten != 14) {
			f_close(&fp);
			return stm32ipl_err_WritingFile;
		}

		res = f_write(&fp, (uint8_t *)bmpinfoheader, sizeof(bmpinfoheader), &byteswritten);
		if (res != FR_OK || byteswritten != 40) {
			f_close(&fp);
			return stm32ipl_err_WritingFile;
		}

		if (width * 3 == row_bytes) {
			for (uint32_t i = 0; i < height; i++) {
				uint32_t scanline = (height - i) * width - (width);
				uint8_t *line = (uint8_t*)(dataToWrite + scanline);
				swapRGB888Channels(line, width);
				res = f_write(&fp, line, width * 3, &byteswritten);
				swapRGB888Channels(line, width);
				if (res != FR_OK || byteswritten != width * 3) {
					f_close(&fp);
					return stm32ipl_err_WritingFile;
				}
			}
		} else {
			for (uint32_t i = 0; i < height; i++) {
				uint32_t scanline = (height - i) * width - (width);
				res = f_write(&fp, dataToWrite + scanline, width * 3, &byteswritten);
				if (res != FR_OK || byteswritten != width * 3) {
					f_close(&fp);
					return stm32ipl_err_WritingFile;
				}

				for (uint32_t j = 0; j < padding; j++) {
					uint8_t zero = 0;
					res = f_write(&fp, &zero, sizeof(uint8_t), &byteswritten);
					if (res != FR_OK || byteswritten != 1) {
						f_close(&fp);
						return stm32ipl_err_WritingFile;
					}
				}
			}
		}

		break;
	}

	default:
		return stm32ipl_err_InvalidParameter;
	};

	f_close(&fp);

	return stm32ipl_err_Ok;
}


/* Save image to PNM file.
 * img: the image to be saved
 * filename: The name of the output file
 * ascii: true if target representation is ASCII, false if it is binary
 * return stm32ipl_err_Ok on success, errors otherwise
 */
static stm32ipl_err_t save_pnm(const image_t *img, const char *filename, bool ascii)
{
	FIL fp;
	FRESULT res;
	uint32_t size = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bit_pixel = 0;
	char message[256] = { "# \n" };

	const uint8_t p2[2] = {0x50, 0x32}; /* P2 */
	const uint8_t p3[2] = {0x50, 0x33}; /* P3 */
	const uint8_t p5[2] = {0x50, 0x35}; /* P5 */
	const uint8_t p6[2] = {0x50, 0x36}; /* P6 */

	uint8_t *pType;
	UINT byteswritten;

	bit_pixel = img->bpp * 8;

	if (f_open(&fp, (const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return stm32ipl_err_OpeningFile;

	switch (bit_pixel) {
		case 8: {
			pType = ascii ? (uint8_t*)p2 : (uint8_t*)p5;
			break;
		}

		case 16:
		case 24: {
			pType = ascii ? (uint8_t*)p3 : (uint8_t*)p6;
			break;
		}

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	/* Write header */
	res = f_write(&fp, pType, 2, &byteswritten);
	if (res != FR_OK || byteswritten != 2) {
		f_close(&fp);
		return stm32ipl_err_WritingFile;
	}

	res = f_write(&fp, (uint8_t *)"\n", 1, &byteswritten);
	if (res != FR_OK || byteswritten != 1) {
		f_close(&fp);
		return stm32ipl_err_WritingFile;
	}

	width  = img->w;
	height = img->h;

	res = f_write(&fp, (uint8_t *)message, 3, &byteswritten);
	if (res != FR_OK || byteswritten != 3) {
		f_close(&fp);
		return stm32ipl_err_WritingFile;
	}

	size = snprintf(message, 255, "%ld %ld\n255\n", width, height);
	res = f_write(&fp, (uint8_t *)message, size, &byteswritten);
	if (res != FR_OK || byteswritten != size) {
		f_close(&fp);
		return stm32ipl_err_WritingFile;
	}

	switch (bit_pixel) {
		case 8: {
			if (ascii) {
				for (uint32_t i = 0; i < height; i++) {
					uint32_t index = i * width;
					for (uint32_t j = 0; j < width; j++) {
						size = snprintf(message, 255, "%d ", img->data[index + j]);
						res = f_write(&fp, (uint8_t *)message, size, &byteswritten);
						if (res != FR_OK || byteswritten != size) {
							f_close(&fp);
							return stm32ipl_err_WritingFile;
						}
					}
					res = f_write(&fp, (uint8_t *)"\n", 1, &byteswritten);
					if (res != FR_OK || byteswritten != 1) {
						f_close(&fp);
						return stm32ipl_err_WritingFile;
					}
				}
			} else {
				size = width * height;
				res = f_write(&fp, (uint8_t *)img->data, size, &byteswritten);
				if (res != FR_OK || byteswritten != size) {
					f_close(&fp);
					return stm32ipl_err_WritingFile;
				}
			}
			break;
		}

		case 16: {
			if (ascii) {
				uint16_t *dataToWrite = (uint16_t *)img->data;
				for (uint32_t i = 0; i < height; i++) {
					uint32_t index = i * width;
					for (uint32_t j = 0; j < width; j++) {
						uint16_t rgb565;
						rgb565 = dataToWrite[index + j];
						uint8_t r = COLOR_RGB565_TO_R8(rgb565);//(uint8_t)((rgb565 & 0xF800) >> 8);
						uint8_t g = COLOR_RGB565_TO_G8(rgb565);//(uint8_t)((rgb565 & 0x07E0) >> 3);
						uint8_t b = COLOR_RGB565_TO_B8(rgb565);//(uint8_t)((rgb565 & 0x001F) << 3);

						size = snprintf(message, 255, "%d %d %d ", r, g, b);
						res = f_write(&fp, (uint8_t *)message, size, &byteswritten);
						if (res != FR_OK || byteswritten != size) {
							f_close(&fp);
							return stm32ipl_err_WritingFile;
						}
					}
					res = f_write(&fp, (uint8_t *)"\n", 1, &byteswritten);
					if (res != FR_OK || byteswritten != 1) {
						f_close(&fp);
						return stm32ipl_err_WritingFile;
					}
				}
			} else {
				uint16_t *dataToWrite = (uint16_t *)img->data;
				for (uint32_t i = 0; i < height; i++) {
					uint32_t index = i * width;
					for (uint32_t j = 0; j < width; j++) {
						uint8_t buff[3];
						uint16_t rgb565;
						rgb565 = dataToWrite[index + j];
						buff[0] = COLOR_RGB565_TO_R8(rgb565);//(uint8_t)((rgb565 & 0xF800) >> 8);
						buff[1] = COLOR_RGB565_TO_G8(rgb565);//(uint8_t)((rgb565 & 0x07E0) >> 3);
						buff[2] = COLOR_RGB565_TO_B8(rgb565);//(uint8_t)((rgb565 & 0x001F) << 3);
						res = f_write(&fp, (uint8_t *)buff, 3, &byteswritten);
						if (res != FR_OK || byteswritten != 3){
							f_close(&fp);
							return stm32ipl_err_WritingFile;
						}
					}
				}
			}
			break;
		}

		case 24: {
			if (ascii) {
				RGB888_t *pixel_rgb888_src = (RGB888_t*)img->data;

				for (uint32_t i = 0; i < height; i++) {
					uint32_t index = i * width;
					for (uint32_t j = 0; j < width; j++) {
						uint32_t offset = index + j;
						size = snprintf(message, 255, "%d %d %d ", pixel_rgb888_src[offset].r, pixel_rgb888_src[offset].g, pixel_rgb888_src[offset].b);
						res = f_write(&fp, (uint8_t *)message, size, &byteswritten);
						if (res != FR_OK || byteswritten != size) {
							f_close(&fp);
							return stm32ipl_err_WritingFile;
						}
					}
					res = f_write(&fp, (uint8_t *)"\n", 1, &byteswritten);
					if (res != FR_OK || byteswritten != 1){
						f_close(&fp);
						return stm32ipl_err_WritingFile;
					}
				}
			} else {
				size = width * height * 3;
				res = f_write(&fp, (uint8_t *)img->data, size, &byteswritten);
				if (res != FR_OK || byteswritten != size) {
					f_close(&fp);
					return stm32ipl_err_WritingFile;
				}
			}
			break;
		}

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	f_close(&fp);

	return stm32ipl_err_Ok;
}


/* Save image to JPEG file.
 * img: the image to be saved
 * filename: The name of the output file
 * return stm32ipl_err_Ok on success, error otherwise
 */
static stm32ipl_err_t save_jpg(const image_t *img, const char *filename)
{
#ifdef STM32IPL_USE_HW_JPEG_CODEC
	return saveJPEGHW(img, filename);
#else
	return saveJPEGSW(img, filename);
#endif
}
