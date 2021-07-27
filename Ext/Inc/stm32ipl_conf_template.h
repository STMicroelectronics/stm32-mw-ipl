/**
 ******************************************************************************
 * @file   stm32ipl_conf_template.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - configuration file
 * This file must be copied to the application folder and modified as follows:
 * - Rename it to 'stm32ipl_conf.h'.
 * - Comment/uncomment the STM32IPL_USE_xxx symbols to disable/enable the
 * associated hardware resource
 * - Change the values relative to the remaining symbols
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

/**
 * @addtogroup STM32IPL_Exported_Defines
 * @{
 */
/* --- NOT CHANGEABLE SECTION - START ---
 * User must not modify values in this section.
 */
#define STM32IPL_JPEG_444_SUBSAMPLING	0   /* 4:4:4 chroma subsampling. */
#define STM32IPL_JPEG_420_SUBSAMPLING	1   /* 4:2:0 chroma subsampling. */
#define STM32IPL_JPEG_422_SUBSAMPLING	2   /* 4:2:2 chroma subsampling. */

#define LCD_FB_ADDR				0xD0000000	/* Address of the LCD frame buffer. */
#define LCD_WIDTH				800			/* LCD width. */
#define LCD_HEIGHT				480			/* LCD height. */
#define LCD_BPP				 	4			/* LCD bytes per pixel. */
#define LCD_FB_SIZE				(LCD_WIDTH * LCD_HEIGHT * LCD_BPP)	/* Size of the LCD frame buffer (bytes). */

#define CAM_FB_ADDR				(LCD_FB_ADDR + LCD_FB_SIZE)	/* Address of the camera frame buffer. */
#define CAM_MAX_WIDTH			640			/* Max camera width. */
#define CAM_MAX_HEIGHT			480			/* Max camera height. */
#define CAM_BPP					2			/* Camera bytes per pixel. */
#define CAM_FB_SIZE				(CAM_MAX_WIDTH * CAM_MAX_HEIGHT * CAM_BPP)	/* Size of the camera frame buffer (bytes). */

#define STM32IPL_BUFFER_ADDR	(CAM_FB_ADDR + CAM_FB_SIZE)					/* Address of the memory buffer assigned to the user. */
/* --- NOT CHANGEABLE SECTION - END --- */


/* --- CHANGEABLE SECTION - START ---
 * User can modify values in this section.
 */
#define STM32IPL_USE_HW_DRAW 			/* Enable hardware accelerated image drawing; comment to disable. */
#define STM32IPL_USE_HW_JPEG_CODEC		/* Enable hardware JPEG codec; comment to disable. */
#define STM32IPL_USE_HW_RGN 			/* Enable hardware random number generator; comment to disable. */
#define STM32IPL_JPEG_QUALITY			90	/* The quality used to encode JPEG images. */
#define STM32IPL_JPEG_SUBSAMPLING		STM32IPL_JPEG_422_SUBSAMPLING	/* The chroma subsampling used to encode JPEG images. */
/* --- CHANGEABLE SECTION - END --- */

#endif /* __STM32IPL_CONF_H_ */

