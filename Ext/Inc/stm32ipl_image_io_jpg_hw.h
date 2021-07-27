/**
 ******************************************************************************
 * @file   stm32ipl_image_io_jpg_hw.h
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

#ifdef STM32IPL_USE_HW_JPEG_CODEC

#ifndef __STM32IPL_IMAGE_IO_JPG_HW_H_
#define __STM32IPL_IMAGE_IO_JPG_HW_H_

#include "stm32ipl.h"
#include "ff.h"

stm32ipl_err_t readJPEGHW(image_t *img, FIL *fp);
stm32ipl_err_t saveJPEGHW(const image_t *img, const char *filename);

#endif /* __STM32IPL_IMAGE_IO_JPG_HW_H_ */

#endif /* STM32IPL_USE_HW_JPEG_CODEC */
