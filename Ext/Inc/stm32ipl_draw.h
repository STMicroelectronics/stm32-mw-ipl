/**
 ******************************************************************************
 * @file   stm32ipl_draw.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - DMA2D drawing function header file
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

#ifndef __STM32IPL_DRAW_H_
#define __STM32IPL_DRAW_H_

#include "stm32ipl.h"

void STM32Ipl_Draw(const image_t *image, uint16_t x, uint16_t y);


#endif /* __STM32IPL_DRAW_H_ */
