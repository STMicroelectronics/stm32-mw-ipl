/**
 ******************************************************************************
 * @file   stm32ipl_draw.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - DMA2D drawing function header file
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

#ifndef __STM32IPL_DRAW_H_
#define __STM32IPL_DRAW_H_

#include "stm32ipl.h"

void STM32Ipl_Draw(const image_t *image, uint16_t x, uint16_t y);

#endif /* __STM32IPL_DRAW_H_ */
