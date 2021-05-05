/**
 ******************************************************************************
 * @file   stm32ipl_image_io.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - image read/write header file
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

#ifndef __STM32IPL_IMAGE_IO_H_
#define __STM32IPL_IMAGE_IO_H_

#include "stm32ipl.h"

stm32ipl_err_t STM32Ipl_ReadImage(image_t *img, const char *filename);
stm32ipl_err_t STM32Ipl_WriteImage(const image_t *img, const char *filename);

#endif /* __STM32IPL_IMAGEIO_H_ */
