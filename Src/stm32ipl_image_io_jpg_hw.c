/**
 ******************************************************************************
 * @file   stm32ipl_image_io_jpg_hw.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - JPEG HW codec for STM32H747I-DISCO
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

#include "stm32ipl_image_io_jpg_hw.h"

#ifdef STM32IPL_USE_HW_JPEG_CODEC

#ifdef USE_STM32H747I_DISCO
#include "stm32h747i_discovery.h"
#include "stm32h7xx_hal_jpeg.h"
#endif /* USE_STM32H747I_DISCO */

#include "jpeg_utils.h"

#define JPEG_HAL_DELAY			5000
#define YCBCR420_MCU_SIZE		384
#define YCBCR422_MCU_SIZE		256
#define YCBCR444_MCU_SIZE		192
#define GRAYSCALE_MCU_SIZE		64
#define CMYK_MCU_SIZE			256

#define YUV_DATA_BUFFER_SIZE	768
#define JPG_DATA_BUFFER_SIZE	512

static uint8_t yuvDataBuffer[YUV_DATA_BUFFER_SIZE];
static uint8_t jpgDataBuffer[JPG_DATA_BUFFER_SIZE];

typedef enum _jpegOp_t
{
	jpegOpDecoding = 0,
	jpegOpEncoding
} jpegOp_t;

typedef struct _jpgCtx_t
{
	JPEG_HandleTypeDef handle;
	FIL *file;
	uint32_t fileOffset;
	JPEG_ConfTypeDef info;
	jpegOp_t operation;
	JPEG_YCbCrToRGB_Convert_Function pYCbCrToRGBFn;
	JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCrFn;
	uint32_t mcuIndex;
	uint32_t mcuTotal;
	uint8_t *rgbDataPtr;
	uint32_t rgbDataSize;
	uint8_t *yuvDataPtr;
	uint32_t yuvDataSize;
	uint8_t *jpgDataPtr;
	uint32_t jpgDataSize;
} jpgCtx_t;

static jpgCtx_t jpgCtx;


/* Initializes the HAL JPEG driver.
 * return	void.
 */
static void JPEG_InitDriver(void)
{
	static uint8_t firstTime = 0;

	/* Clear the context. */
	memset(&jpgCtx, 0, sizeof(jpgCtx_t));

	jpgCtx.handle.Instance = JPEG;

	HAL_JPEG_Init(&(jpgCtx.handle));

	if (!firstTime) {
		firstTime = 1;
		JPEG_InitColorTables();
	}
}

/* De-initializes the HAL JPEG driver.
 * return	void.
 */
static void JPEG_DeInitDriver(void)
{
	/* Deinit the HAL JPEG driver. */
	HAL_JPEG_DeInit(&(jpgCtx.handle));

	/* Clear the context. */
	memset(&jpgCtx, 0, sizeof(jpgCtx_t));
}

/* Error handler.
 * return	void.
 */
static void JPEG_ErrorHandler(void)
{
	HAL_JPEG_Abort(&(jpgCtx.handle));
}

/*
 * Encodes the given image as JPEG and save it to file; the supported source image formats are:
 * Grayscale and RGB565.
 * hjpg			Pointer to the JPEG handle.
 * img			Image to be saved; both its width and height must be multiple of 8, except the following cases:
 * - if the target is YcbCr4:2:0, then the height must be multiple of 16.
 * - if the target is YcbCr4:2:0 or YcbCr4:2:2, then the width must be multiple of 16.
 * colorSpace	Color space; it can be one of these values:
 * JPEG_GRAYSCALE_COLORSPACE, JPEG_YCBCR_COLORSPACE, JPEG_CMYK_COLORSPACE.
 * chromaSS		Sub-sampling schema, valid only if colorSpace is JPEG_YCBCR_COLORSPACE.
 * it can be one of these values: JPEG_420_SUBSAMPLING, JPEG_422_SUBSAMPLING, JPEG_444_SUBSAMPLING
 * quality		Quality; it can assume any integer value between JPEG_IMAGE_QUALITY_MIN
 * and JPEG_IMAGE_QUALITY_MAX.
 * return		stm32ipl_err_Ok on success, error otherwise.
 */
static stm32ipl_err_t JPEG_Encode(FIL *fp, const image_t *img, uint32_t colorSpace, uint32_t chromaSS, uint8_t quality)
{
	HAL_StatusTypeDef res;
	uint32_t yuvBytesRead;

	if (!fp || !img)
		return stm32ipl_err_InvalidParameter;

	if (img->bpp != IMAGE_BPP_RGB565 && img->bpp != IMAGE_BPP_GRAYSCALE)
		return stm32ipl_err_UnsupportedFormat;

	if (colorSpace != JPEG_GRAYSCALE_COLORSPACE && colorSpace != JPEG_YCBCR_COLORSPACE
			&& colorSpace != JPEG_CMYK_COLORSPACE)
		return stm32ipl_err_InvalidParameter;

	if (chromaSS != JPEG_420_SUBSAMPLING && chromaSS != JPEG_422_SUBSAMPLING && chromaSS != JPEG_444_SUBSAMPLING)
		return stm32ipl_err_InvalidParameter;

	/* Check if image size meets the requirements. */
	if (((img->w % 8) != 0) || ((img->h % 8) != 0)
			|| (((img->w % 16) != 0) && (colorSpace == JPEG_YCBCR_COLORSPACE) && (chromaSS != JPEG_444_SUBSAMPLING))
			|| (((img->h % 16) != 0) && (colorSpace == JPEG_YCBCR_COLORSPACE) && (chromaSS == JPEG_420_SUBSAMPLING)))
		return stm32ipl_err_WrongSize;

	if (quality < JPEG_IMAGE_QUALITY_MIN || quality > JPEG_IMAGE_QUALITY_MAX)
		return stm32ipl_err_InvalidParameter;

	jpgCtx.file = fp;
	jpgCtx.fileOffset = 0;
	jpgCtx.info.ColorSpace = colorSpace;
	jpgCtx.info.ChromaSubsampling = chromaSS;
	jpgCtx.info.ImageHeight = img->h;
	jpgCtx.info.ImageWidth = img->w;
	jpgCtx.info.ImageQuality = quality;
	jpgCtx.operation = jpegOpEncoding;

	jpgCtx.mcuIndex = 0;
	jpgCtx.mcuTotal = 0;

	jpgCtx.jpgDataPtr = jpgDataBuffer;
	jpgCtx.jpgDataSize = JPG_DATA_BUFFER_SIZE;

	if (colorSpace == JPEG_YCBCR_COLORSPACE) {
		if (chromaSS == JPEG_420_SUBSAMPLING) {
			jpgCtx.rgbDataSize = 512;
		} else
			if (chromaSS == JPEG_422_SUBSAMPLING) {
				jpgCtx.rgbDataSize = 256;
			} else
				if (chromaSS == JPEG_444_SUBSAMPLING) {
					jpgCtx.rgbDataSize = 128;
				}
	} else
		if (colorSpace == JPEG_GRAYSCALE_COLORSPACE) {
			jpgCtx.rgbDataSize = 128;
		} else
			if (colorSpace == JPEG_CMYK_COLORSPACE) {
				jpgCtx.rgbDataSize = 128;
			}

	if (HAL_ERROR == JPEG_GetEncodeColorConvertFunc(&jpgCtx.info, &jpgCtx.pRGBToYCbCrFn, &jpgCtx.mcuTotal))
		return stm32ipl_err_OpNotCompleted;

	if (HAL_ERROR == HAL_JPEG_ConfigEncoding(&jpgCtx.handle, &jpgCtx.info))
		return stm32ipl_err_OpNotCompleted;

	jpgCtx.rgbDataPtr = img->data;

	jpgCtx.yuvDataSize = YUV_DATA_BUFFER_SIZE;
	jpgCtx.yuvDataPtr = yuvDataBuffer;

	yuvBytesRead = 0;

	jpgCtx.mcuIndex += jpgCtx.pRGBToYCbCrFn(jpgCtx.rgbDataPtr, jpgCtx.yuvDataPtr, jpgCtx.mcuIndex, jpgCtx.rgbDataSize,
			&yuvBytesRead);

	res = HAL_JPEG_Encode(&jpgCtx.handle, jpgCtx.yuvDataPtr, yuvBytesRead, jpgCtx.jpgDataPtr, JPG_DATA_BUFFER_SIZE,
			JPEG_HAL_DELAY);

	return res == HAL_OK ? stm32ipl_err_Ok : stm32ipl_err_OpNotCompleted;
}

/*
 * Called by the decoder when the JPEG file info is decoded.
 * hjpg		Pointer to the JPEG handle.
 * pInfo	Pointer to the structure containing the decoded info.
 * return	void.
 */
void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpg, JPEG_ConfTypeDef *pInfo)
{
	/* In case of grayscale JPEG, the output image will be grayscale as well, otherwise it will be RGB565. */
	/* Allocate memory to contain the decoded image. */
	jpgCtx.rgbDataSize = pInfo->ImageWidth * pInfo->ImageHeight
			* ((pInfo->ColorSpace == JPEG_GRAYSCALE_COLORSPACE) ? IMAGE_BPP_GRAYSCALE : IMAGE_BPP_RGB565);
	jpgCtx.rgbDataPtr = xalloc(jpgCtx.rgbDataSize);
	if (!jpgCtx.rgbDataPtr)
		JPEG_ErrorHandler();

	jpgCtx.info = *pInfo;

	JPEG_GetDecodeColorConvertFunc(pInfo, &jpgCtx.pYCbCrToRGBFn, &jpgCtx.mcuTotal);
}

/*
 * Called by the peripheral when it needs new input data.
 * hjpg				Pointer to the JPEG handle.
 * processedBytes	Number of bytes processed by the peripheral.
 * return			void.
 */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpg, uint32_t processedBytes)
{
	/* Decoding. */
	if (jpgCtx.operation == jpegOpDecoding) {
		HAL_JPEG_Pause(hjpg, JPEG_PAUSE_RESUME_INPUT);

		if (processedBytes != jpgCtx.jpgDataSize) {
			jpgCtx.fileOffset = jpgCtx.fileOffset - jpgCtx.jpgDataSize + processedBytes;

			if (f_lseek(jpgCtx.file, jpgCtx.fileOffset) != FR_OK)
				JPEG_ErrorHandler();
		}

		if (f_read(jpgCtx.file, jpgCtx.jpgDataPtr, JPG_DATA_BUFFER_SIZE, (UINT*)(&jpgCtx.jpgDataSize)) == FR_OK) {
			jpgCtx.fileOffset += jpgCtx.jpgDataSize;

			HAL_JPEG_ConfigInputBuffer(hjpg, jpgCtx.jpgDataPtr, jpgCtx.jpgDataSize);

			HAL_JPEG_Resume(hjpg, JPEG_PAUSE_RESUME_INPUT);

		} else
			JPEG_ErrorHandler();
	} else {
		/* Encoding. */
		if (jpgCtx.mcuIndex < jpgCtx.mcuTotal) {
			uint32_t yuvBytesRead;

			HAL_JPEG_Pause(hjpg, JPEG_PAUSE_RESUME_INPUT);

			jpgCtx.mcuIndex += jpgCtx.pRGBToYCbCrFn(jpgCtx.rgbDataPtr, jpgCtx.yuvDataPtr, jpgCtx.mcuIndex,
					jpgCtx.rgbDataSize, &yuvBytesRead);

			HAL_JPEG_ConfigInputBuffer(hjpg, jpgCtx.yuvDataPtr, yuvBytesRead);

			HAL_JPEG_Resume(hjpg, JPEG_PAUSE_RESUME_INPUT);
		} else
			HAL_JPEG_ConfigInputBuffer(hjpg, jpgCtx.rgbDataPtr, 0);
	}
}

/*
 * Called by the peripheral when it has new output data.
 * hjpg			Pointer to the JPEG handle.
 * outDataPtr	Pointer to the output data buffer.
 * outDataSize	Length of the output data buffer (in bytes).
 * return		void.
 */
void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpg, uint8_t *outDataPtr, uint32_t outDataSize)
{
	/* Decoding. */
	if (jpgCtx.operation == jpegOpDecoding) {
		/*SCB_InvalidateDCache_by_Addr((uint32_t*)outDataPtr, outDataSize);*/
		HAL_JPEG_Pause(hjpg, JPEG_PAUSE_RESUME_OUTPUT);

		jpgCtx.mcuIndex += jpgCtx.pYCbCrToRGBFn(outDataPtr, jpgCtx.rgbDataPtr, jpgCtx.mcuIndex, outDataSize, NULL);

		HAL_JPEG_ConfigOutputBuffer(hjpg, jpgCtx.yuvDataPtr, jpgCtx.yuvDataSize);

		HAL_JPEG_Resume(hjpg, JPEG_PAUSE_RESUME_OUTPUT);
	} else {
		/* Encoding. */
		uint32_t bytesWritten;

		HAL_JPEG_Pause(hjpg, JPEG_PAUSE_RESUME_OUTPUT);

		if (f_write(jpgCtx.file, outDataPtr, outDataSize, (UINT*)(&bytesWritten)) == FR_OK) {
			HAL_JPEG_ConfigOutputBuffer(hjpg, jpgCtx.jpgDataPtr, JPG_DATA_BUFFER_SIZE);

			HAL_JPEG_Resume(hjpg, JPEG_PAUSE_RESUME_OUTPUT);
		} else
			JPEG_ErrorHandler();
	}
}

/*
 * Called by the peripheral in case of errors.
 * hjpg		Pointer to the JPEG handle.
 * return	void.
 */
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpg)
{
	/* HAL_JPEG_GetError(); */
	JPEG_ErrorHandler();
}

/*
 * Reads and decodes a JPEG file by using the STM32 hardware accelerator. The decoded image
 * will be Grayscale or RGB565 depending on the file content.
 * img		Decoded image; the image data buffer is allocated internally; it is up to the caller
 * to release the image data when done with it with STM32Ipl_ReleaseData().
 * fp		Pointer to the file object.
 * return	stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t readJPEGHW(image_t *img, FIL *fp)
{
	stm32ipl_err_t res;
	HAL_StatusTypeDef ret;

	if (!img || !fp)
		return stm32ipl_err_InvalidParameter;

	STM32Ipl_Init(img, 0, 0, (image_bpp_t)0, 0);

	if (f_lseek(fp, 0) != FR_OK)
		return stm32ipl_err_SeekingFile;

	JPEG_InitDriver();

	jpgCtx.file = fp;
	jpgCtx.operation = jpegOpDecoding;
	jpgCtx.mcuIndex = 0;
	jpgCtx.mcuTotal = 0;
	jpgCtx.rgbDataPtr = 0;
	jpgCtx.rgbDataSize = 0;
	jpgCtx.yuvDataPtr = yuvDataBuffer;
	jpgCtx.yuvDataSize = YUV_DATA_BUFFER_SIZE;
	jpgCtx.jpgDataPtr = jpgDataBuffer;
	jpgCtx.jpgDataSize = JPG_DATA_BUFFER_SIZE;

	/* Read from JPEG file and fill the input buffer. */
	if (f_read(jpgCtx.file, jpgCtx.jpgDataPtr, JPG_DATA_BUFFER_SIZE, (UINT*)(&jpgCtx.jpgDataSize)) != FR_OK)
		return stm32ipl_err_ReadingFile;

	/* Update the file offset. */
	jpgCtx.fileOffset = jpgCtx.jpgDataSize;

	/* Start JPEG decoding with polling (blocking) method. */
	ret = HAL_JPEG_Decode(&jpgCtx.handle, jpgCtx.jpgDataPtr, jpgCtx.jpgDataSize, jpgCtx.yuvDataPtr, jpgCtx.yuvDataSize,
	JPEG_HAL_DELAY);
	res = (ret == HAL_OK) ? stm32ipl_err_Ok : stm32ipl_err_OpNotCompleted;

	if (res == stm32ipl_err_Ok)
		STM32Ipl_Init(img, jpgCtx.info.ImageWidth, jpgCtx.info.ImageHeight,
				(jpgCtx.info.ColorSpace == JPEG_GRAYSCALE_COLORSPACE) ? IMAGE_BPP_GRAYSCALE : IMAGE_BPP_RGB565,
				jpgCtx.rgbDataPtr);

	JPEG_DeInitDriver();

	return res;
}

/*
 * Encodes the given image to a JPEG file by using the STM32 hardware accelerator: the supported
 * source image formats are: Grayscale and RGB565. The current implementation of jpeg_utils does not support
 * more that one RGB format at run-time: the actual format used (RGB565 or RGB888) is chosen at compile time.
 * Moreover the jpeg_utils mechanism works on a full image buffer; this means that a line based format conversion
 * cannot be used, therefore RGB888 images must be explicitly converted to RGB565 before this function is called.
 * img		Image to be encoded. Both image width and height must be multiple of 8, except the following cases:
 * - if the target is YcbCr4:2:0, then the height must be multiple of 16.
 * - if the target is YcbCr4:2:0 or YcbCr4:2:2, then the width must be multiple of 16.
 * return	stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t saveJPEGHW(const image_t *img, const char *filename)
{
	stm32ipl_err_t res;
	FIL fp;

	if (!img || !filename || !img->data)
		return stm32ipl_err_InvalidParameter;

	if (f_open(&fp, (const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return stm32ipl_err_OpeningFile;

	JPEG_InitDriver();

	/* A grayscale image always needs 4:4:4 subsampling. */
	res = JPEG_Encode(&fp, img, img->bpp == IMAGE_BPP_GRAYSCALE ? JPEG_GRAYSCALE_COLORSPACE : JPEG_YCBCR_COLORSPACE,
			img->bpp == IMAGE_BPP_GRAYSCALE ? STM32IPL_JPEG_444_SUBSAMPLING : STM32IPL_JPEG_SUBSAMPLING,
			STM32IPL_JPEG_QUALITY);

	JPEG_DeInitDriver();

	f_close(&fp);

	return res;
}

#endif /* STM32IPL_USE_HW_JPEG_CODEC */
