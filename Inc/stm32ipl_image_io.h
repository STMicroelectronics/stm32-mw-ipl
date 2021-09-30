/**
 ******************************************************************************
 * @file   stm32ipl_image_io.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - image read/write header file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef __STM32IPL_IMAGE_IO_H_
#define __STM32IPL_IMAGE_IO_H_

#include "stm32ipl.h"

stm32ipl_err_t STM32Ipl_ReadImage(image_t *img, const char *filename);
stm32ipl_err_t STM32Ipl_WriteImage(const image_t *img, const char *filename);

#endif /* __STM32IPL_IMAGEIO_H_ */
