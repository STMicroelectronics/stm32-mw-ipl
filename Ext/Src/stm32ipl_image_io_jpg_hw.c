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

#include "stm32h747i_discovery.h"
#include "stm32h7xx_hal_jpeg.h"
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


typedef enum _jpegOp_t {
	jpegOpDecoding = 0,
	jpegOpEncoding
} jpegOp_t;


typedef struct _jpgCtx_t {
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

#ifdef STM32IPL_PERF
#include "perf.h"
extern perfTimes_t perf;
#endif /* STM32IPL_PERF */

//static void JPEG_InitDriver(void);
//static void JPEG_DeInitDriver(void);
//static void JPEG_ErrorHandler(void);
//static stm32ipl_err_t JPEG_Encode(FIL *fp, const image_t *img, uint32_t colorSpace, uint32_t chromaSubsampling, uint8_t quality);


/* Init the HAL JPEG driver. */
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


/* Deinit the HAL JPEG driver. */
static void JPEG_DeInitDriver(void)
{
	/* Uninit the HAL JPEG driver. */
	HAL_JPEG_DeInit(&(jpgCtx.handle));

	/* Clear the context. */
	memset(&jpgCtx, 0, sizeof(jpgCtx_t));
}


/* Error handler. */
static void JPEG_ErrorHandler(void)
{
	HAL_JPEG_Abort(&(jpgCtx.handle));
}


/*
 * Encode the given image as JPEG and save it to file
 * hjpg: JPEG handle pointer
 * img: the image to be saved (must have RGB565 or grayscale format); both its width and height
 * must be multiple of 8, except the following cases:
 * - if the target is YcbCr4:2:0, then the height must be multiple of 16
 * - if the target is YcbCr4:2:0 or YcbCr4:2:2, then the width must be multiple of 16.
 * colorSpace: the chosen color space; it can be one of these values:
 * JPEG_GRAYSCALE_COLORSPACE, JPEG_YCBCR_COLORSPACE, JPEG_CMYK_COLORSPACE
 * chromaSubsampling: the chosen sub-sampling schema, valid only if colorSpace is
 * JPEG_YCBCR_COLORSPACE; it can be one of these values:
 * JPEG_420_SUBSAMPLING, JPEG_422_SUBSAMPLING, JPEG_444_SUBSAMPLING
 * quality: the chosen quality; it can assume any integer value between
 * JPEG_IMAGE_QUALITY_MIN and JPEG_IMAGE_QUALITY_MAX
 * return stm32ipl_err_Ok on success, error otherwise
 */
static stm32ipl_err_t JPEG_Encode(FIL *fp, const image_t *img,
		uint32_t colorSpace, uint32_t chromaSubsampling, uint8_t quality)
{
	HAL_StatusTypeDef res;
	uint32_t yuvBytesRead;

	if (!fp || !img)
		return stm32ipl_err_InvalidParameter;

	if (img->bpp != IMAGE_BPP_RGB565 && img->bpp != IMAGE_BPP_GRAYSCALE)
		return stm32ipl_err_UnsupportedFormat;

	if (colorSpace != JPEG_GRAYSCALE_COLORSPACE &&
		colorSpace != JPEG_YCBCR_COLORSPACE &&
		colorSpace != JPEG_CMYK_COLORSPACE)
		return stm32ipl_err_InvalidParameter;

	if (chromaSubsampling != JPEG_420_SUBSAMPLING &&
		chromaSubsampling != JPEG_422_SUBSAMPLING &&
		chromaSubsampling != JPEG_444_SUBSAMPLING)
		return stm32ipl_err_InvalidParameter;

	/* Check if image size meets the requirements. */
	if (((img->w % 8)  != 0) || ((img->h % 8) != 0) ||
	   (((img->w % 16) != 0) && (colorSpace == JPEG_YCBCR_COLORSPACE)
			   && (chromaSubsampling != JPEG_444_SUBSAMPLING)) ||
	   (((img->h % 16) != 0) && (colorSpace == JPEG_YCBCR_COLORSPACE) &&
			   (chromaSubsampling == JPEG_420_SUBSAMPLING)))
		return stm32ipl_err_WrongSize;

	if (quality < JPEG_IMAGE_QUALITY_MIN || quality > JPEG_IMAGE_QUALITY_MAX)
		return stm32ipl_err_InvalidParameter;

	jpgCtx.file = fp;
	jpgCtx.fileOffset = 0;
	jpgCtx.info.ColorSpace = colorSpace;
	jpgCtx.info.ChromaSubsampling = chromaSubsampling;
	jpgCtx.info.ImageHeight = img->h;
	jpgCtx.info.ImageWidth = img->w;
	jpgCtx.info.ImageQuality = quality;
	jpgCtx.operation = jpegOpEncoding;

	jpgCtx.mcuIndex = 0;
	jpgCtx.mcuTotal = 0;

	jpgCtx.jpgDataPtr  = jpgDataBuffer;
	jpgCtx.jpgDataSize = JPG_DATA_BUFFER_SIZE;

	if (colorSpace == JPEG_YCBCR_COLORSPACE) {
		if (chromaSubsampling == JPEG_420_SUBSAMPLING) {
			jpgCtx.rgbDataSize = 512;
		} else
		if (chromaSubsampling == JPEG_422_SUBSAMPLING) {
			jpgCtx.rgbDataSize = 256;
		} else
		if (chromaSubsampling == JPEG_444_SUBSAMPLING) {
			jpgCtx.rgbDataSize = 128;
		}
	} else
	if (colorSpace == JPEG_GRAYSCALE_COLORSPACE) {
		jpgCtx.rgbDataSize = 128;
	}
	else
	if (colorSpace == JPEG_CMYK_COLORSPACE) {
		jpgCtx.rgbDataSize = 128;
	}

	if (HAL_ERROR == JPEG_GetEncodeColorConvertFunc(&jpgCtx.info, &jpgCtx.pRGBToYCbCrFn, &jpgCtx.mcuTotal))
		return stm32ipl_err_OpNotCompleted;

	if (HAL_ERROR == HAL_JPEG_ConfigEncoding(&jpgCtx.handle, &jpgCtx.info))
		return stm32ipl_err_OpNotCompleted;

	jpgCtx.rgbDataPtr  = img->data;
	jpgCtx.yuvDataSize = YUV_DATA_BUFFER_SIZE;
	jpgCtx.yuvDataPtr  = yuvDataBuffer;

	yuvBytesRead = 0;

#ifdef STM32IPL_PERF
	uint32_t tickStart = HAL_GetTick();
#endif /* STM32IPL_PERF */
	jpgCtx.mcuIndex += jpgCtx.pRGBToYCbCrFn(jpgCtx.rgbDataPtr, jpgCtx.yuvDataPtr, jpgCtx.mcuIndex, jpgCtx.rgbDataSize, &yuvBytesRead);
#ifdef STM32IPL_PERF
	perf.encConv += HAL_GetTick() - tickStart;
#endif /* STM32IPL_PERF */
	res = HAL_JPEG_Encode(&jpgCtx.handle, jpgCtx.yuvDataPtr, yuvBytesRead, jpgCtx.jpgDataPtr, JPG_DATA_BUFFER_SIZE, JPEG_HAL_DELAY);

	return res == HAL_OK ? stm32ipl_err_Ok : stm32ipl_err_OpNotCompleted;
}


/* Called by the decoder when the JPEG file info is decoded.
   hjpg: the pointer to the JPEG handle
   pInfo: the pointer to the structure containing the decoded info
   */
void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpg, JPEG_ConfTypeDef *pInfo)
{
	/* In case of grayscale JPEG, the output image will be grayscale as well, otherwise it will be RGB565. */
	/* Allocate memory to contain the decoded image. */
	jpgCtx.rgbDataSize = pInfo->ImageWidth * pInfo->ImageHeight *
			((pInfo->ColorSpace == JPEG_GRAYSCALE_COLORSPACE) ? IMAGE_BPP_GRAYSCALE : IMAGE_BPP_RGB565);
	jpgCtx.rgbDataPtr = xalloc(jpgCtx.rgbDataSize);
	if (!jpgCtx.rgbDataPtr)
		JPEG_ErrorHandler();

	jpgCtx.info = *pInfo;

	JPEG_GetDecodeColorConvertFunc(pInfo, &jpgCtx.pYCbCrToRGBFn, &jpgCtx.mcuTotal);
}


/* Called by the peripheral when it needs new input data.
   hjpg: the pointer to the JPEG handle
   processedBytes: the number of bytes processed by the peripheral
   */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpg, uint32_t processedBytes)
{
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
		if (jpgCtx.mcuIndex < jpgCtx.mcuTotal) {
			uint32_t yuvBytesRead;

			HAL_JPEG_Pause(hjpg, JPEG_PAUSE_RESUME_INPUT);

#ifdef STM32IPL_PERF
			uint32_t tickStart = HAL_GetTick();
#endif /* STM32IPL_PERF */
			jpgCtx.mcuIndex += jpgCtx.pRGBToYCbCrFn(jpgCtx.rgbDataPtr, jpgCtx.yuvDataPtr, jpgCtx.mcuIndex, jpgCtx.rgbDataSize, &yuvBytesRead);
#ifdef STM32IPL_PERF
                        perf.encConv += HAL_GetTick() - tickStart;
#endif /* STM32IPL_PERF */
			HAL_JPEG_ConfigInputBuffer(hjpg, jpgCtx.yuvDataPtr, yuvBytesRead);

			HAL_JPEG_Resume(hjpg, JPEG_PAUSE_RESUME_INPUT);
		} else
			HAL_JPEG_ConfigInputBuffer(hjpg, jpgCtx.rgbDataPtr, 0);
	}
}


/* Called by the peripheral when it has new output data.
   hjpg: the pointer to the JPEG handle
   outDataPtr: the pointer to the output data buffer
   outDataSize: the length of the output data buffer (in bytes)
*/
void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpg, uint8_t *outDataPtr, uint32_t outDataSize)
{
	/* Decoding. */
	if (jpgCtx.operation == jpegOpDecoding) {
		/*SCB_InvalidateDCache_by_Addr((uint32_t*)outDataPtr, outDataSize);*/
		HAL_JPEG_Pause(hjpg, JPEG_PAUSE_RESUME_OUTPUT);

		jpgCtx.mcuIndex += jpgCtx.pYCbCrToRGBFn(outDataPtr, jpgCtx.rgbDataPtr,
				jpgCtx.mcuIndex, outDataSize, NULL);

		HAL_JPEG_ConfigOutputBuffer(hjpg, jpgCtx.yuvDataPtr, jpgCtx.yuvDataSize);

		HAL_JPEG_Resume(hjpg, JPEG_PAUSE_RESUME_OUTPUT);
	} else {
		/* Encoding. */
		uint32_t bytesWritten;

		HAL_JPEG_Pause(hjpg, JPEG_PAUSE_RESUME_OUTPUT);

#ifdef STM32IPL_PERF
		uint32_t tickStart = HAL_GetTick();
#endif /* STM32IPL_PERF */
		if (f_write(jpgCtx.file, outDataPtr, outDataSize, (UINT*)(&bytesWritten)) == FR_OK) {
#ifdef STM32IPL_PERF
                        perf.encFileIO += HAL_GetTick() - tickStart;
			perf.bytesWritten += bytesWritten;
			perf.writeCount++;
#endif /* STM32IPL_PERF */
			HAL_JPEG_ConfigOutputBuffer(hjpg, jpgCtx.jpgDataPtr, JPG_DATA_BUFFER_SIZE);

			HAL_JPEG_Resume(hjpg, JPEG_PAUSE_RESUME_OUTPUT);
		} else
			JPEG_ErrorHandler();
	}
}


/* Called by the peripheral in case of errors.
   hjpg: the pointer to the JPEG handle
*/
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpg)
{
	/*HAL_JPEG_GetError();*/
	JPEG_ErrorHandler();
}


/* Read and decode a JPEG file and return it into a image structure pointed by img;
 * the STM32 hardware decoder is used
 * img: the decoded image; image data is allocated internally; it is up to the caller
 * to release the image data when done with it
 * fp: the pointer to the file object
 * return stm32ipl_err_Ok on success, error otherwise
 * */
stm32ipl_err_t readJPEGHW(image_t *img, FIL* fp)
{
	stm32ipl_err_t res;
	HAL_StatusTypeDef ret;

	if (!img || !fp)
		return stm32ipl_err_InvalidParameter;

	image_init(img, 0, 0, 0, 0);

	if (f_lseek(fp, 0) != FR_OK)
		return stm32ipl_err_SeekingFile;

	JPEG_InitDriver();

	jpgCtx.file        = fp;
	jpgCtx.operation   = jpegOpDecoding;
	jpgCtx.mcuIndex    = 0;
	jpgCtx.mcuTotal    = 0;
	jpgCtx.rgbDataPtr  = 0;
	jpgCtx.rgbDataSize = 0;
	jpgCtx.yuvDataPtr  = yuvDataBuffer;
	jpgCtx.yuvDataSize = YUV_DATA_BUFFER_SIZE;
	jpgCtx.jpgDataPtr  = jpgDataBuffer;
	jpgCtx.jpgDataSize = JPG_DATA_BUFFER_SIZE;

	/* Read from JPEG file and fill the input buffer. */
	if (f_read(jpgCtx.file, jpgCtx.jpgDataPtr, JPG_DATA_BUFFER_SIZE, (UINT*)(&jpgCtx.jpgDataSize)) != FR_OK)
		return stm32ipl_err_ReadingFile;

	/* Update the file offset. */
	jpgCtx.fileOffset = jpgCtx.jpgDataSize;

	/* Start JPEG decoding with polling (blocking) method. */
	ret = HAL_JPEG_Decode(&jpgCtx.handle, jpgCtx.jpgDataPtr, jpgCtx.jpgDataSize, jpgCtx.yuvDataPtr, jpgCtx.yuvDataSize, JPEG_HAL_DELAY);
	res = (ret == HAL_OK) ? stm32ipl_err_Ok : stm32ipl_err_OpNotCompleted;

	if (res == stm32ipl_err_Ok)
		image_init(img, jpgCtx.info.ImageWidth, jpgCtx.info.ImageHeight, (jpgCtx.info.ColorSpace == JPEG_GRAYSCALE_COLORSPACE) ? IMAGE_BPP_GRAYSCALE : IMAGE_BPP_RGB565, jpgCtx.rgbDataPtr);

	JPEG_DeInitDriver();

	return res;
}


/* Encode the given image as JPEG and save it to file
 * img: the image to be saved (must have RGB565 or grayscale format); both its width and height
 * must be multiple of 8, except the following cases:
 * - if the target is YcbCr4:2:0, then the height must be multiple of 16
 * - if the target is YcbCr4:2:0 or YcbCr4:2:2, then the width must be multiple of 16.
 * return stm32ipl_err_Ok on success, error otherwise
 */
stm32ipl_err_t saveJPEGHW(const image_t *img, const char *filename)
{
	stm32ipl_err_t res;
	FIL fp;

	if (!img || !filename || !img->data)
		return stm32ipl_err_InvalidParameter;

#ifdef STM32IPL_PERF
	uint32_t tickStart = HAL_GetTick();
#endif /* STM32IPL_PERF */
	if (f_open(&fp, (const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return stm32ipl_err_OpeningFile;
#ifdef STM32IPL_PERF
	perf.encFileIO += HAL_GetTick() - tickStart;
#endif /* STM32IPL_PERF */
	JPEG_InitDriver();

	/* A grayscale image always needs 4:4:4 subsampling. */
	res = JPEG_Encode(&fp, img,
			img->bpp == IMAGE_BPP_GRAYSCALE ? JPEG_GRAYSCALE_COLORSPACE : JPEG_YCBCR_COLORSPACE,
			img->bpp == IMAGE_BPP_GRAYSCALE ? STM32IPL_JPEG_444_SUBSAMPLING : STM32IPL_JPEG_SUBSAMPLING, STM32IPL_JPEG_QUALITY);

	JPEG_DeInitDriver();
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
