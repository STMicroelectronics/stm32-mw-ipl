/**
 ******************************************************************************
 * @file   stm32ipl_conf.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - configuration file
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

#ifndef __STM32IPL_CONF_H_
#define __STM32IPL_CONF_H_

#include "stm32ipl_def.h"

/* Platform specific settings. */
#ifdef USE_STM32H747I_DISCO
#define STM32IPL_LCD_FB_ADDR		0xD0000000	/* Address of the LCD frame buffer. */
#define STM32IPL_LCD_WIDTH			800			/* LCD width. */
#define STM32IPL_LCD_HEIGHT			480			/* LCD height. */
#define STM32IPL_LCD_BPP			4			/* LCD bytes per pixel. */
#define STM32IPL_LCD_FB_SIZE		(STM32IPL_LCD_WIDTH * STM32IPL_LCD_HEIGHT * STM32IPL_LCD_BPP)	/* LCD frame buffer size (bytes). */

#define STM32IPL_CAM_FB_ADDR		(STM32IPL_LCD_FB_ADDR + STM32IPL_LCD_FB_SIZE)	/* Camera frame buffer address. */
#define STM32IPL_CAM_WIDTH			640			/* Camera width. */
#define STM32IPL_CAM_HEIGHT			480			/* Camera height. */
#define STM32IPL_CAM_BPP			2			/* Camera bytes per pixel. */
#define STM32IPL_CAM_FB_SIZE		(STM32IPL_CAM_WIDTH * STM32IPL_CAM_HEIGHT * STM32IPL_CAM_BPP)	/* Camera frame buffer size (bytes). */

#define STM32IPL_EXT_MEM_ADDR		(STM32IPL_CAM_FB_ADDR + STM32IPL_CAM_FB_SIZE)	/* Address of the external memory assigned to STM32IPL. */

#define STM32IPL_EXT_BUFFER_SIZE	(1024 * 2000)	/* Size of the external memory buffer reserved to STM32IPL. */
#define STM32IPL_INT_BUFFER_SIZE	(1024 * 470)	/* Size of the internal memory buffer reserved to STM32IPL. */
#define STM32IPL_USE_HW_DRAW 		/* Enable hardware accelerated image drawing; comment to disable. */
#define STM32IPL_USE_HW_JPEG_CODEC	/* Enable hardware JPEG codec; comment to disable. */
#define STM32IPL_USE_HW_RGN 		/* Enable hardware random number generator; comment to disable. */
//#define STM32IPL_USE_INT_RAM		/* Enable usage of internal memory for buffers allocated by STM32IPL; comment to force usage of external memory. */
#endif /* USE_STM32H747I_DISCO */


/* General settings. */
#define STM32IPL_JPEG_QUALITY		90	/* The quality used to encode JPEG images. */
#define STM32IPL_JPEG_SUBSAMPLING	STM32IPL_JPEG_422_SUBSAMPLING	/* The chroma subsampling used to encode JPEG images. */


/* Library modules enablers. */
#define STM32IPL_ENABLE_IMAGE_IO			/* Enable image IO functions; comment to disable. */
#define STM32IPL_ENABLE_OBJECT_DETECTION	/* Enable object detection; comment to disable. */
#define STM32IPL_USE_FRONTAL_FACE_CASCADE	/* Use frontal face cascade; comment to do not use. */
#define STM32IPL_USE_EYE_CASCADE			/* Use eye cascade; comment to do not use. */

#endif /* __STM32IPL_CONF_H_ */
