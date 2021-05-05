/**
 ******************************************************************************
 * @file   stm32ipl_conf.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - configuration file
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

#ifndef __STM32IPL_CONF_H_
#define __STM32IPL_CONF_H_

/** @addtogroup STM32IPL_Exported_Defines
  * @{
  */
#define STM32IPL_USE_HW_DRAW				/* Enable hardware accelerated image drawing; comment to disable. */

#define STM32IPL_USE_HW_JPEG_CODEC		/* Enable hardware JPEG codec; comment to disable. */

#define STM32IPL_JPEG_QUALITY			90	/* The quality used to encode JPEG images. */

#define STM32IPL_JPEG_444_SUBSAMPLING	0   /* 4:4:4 chroma subsampling. */
#define STM32IPL_JPEG_420_SUBSAMPLING	1   /* 4:2:0 chroma subsampling. */
#define STM32IPL_JPEG_422_SUBSAMPLING	2   /* 4:2:2 chroma subsampling. */

#define STM32IPL_JPEG_SUBSAMPLING		STM32IPL_JPEG_422_SUBSAMPLING	/* The chroma subsampling used to encode JPEG images. */

#endif /* __STM32IPL_CONF_H_ */


