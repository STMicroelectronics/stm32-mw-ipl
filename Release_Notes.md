# Release Notes for STM32 Image Processing Library

## Purpose

The STM32 Image Processing Library is a C-based software library for STM32 microcontrollers, offering specialized features to facilitate the development of computer vision applications.

## Update history

### V1.2.0 / September 2025

- Implemented Dewarping functionality.
- Enhanced Binary function for RGB888 format and added an MVE-optimized implementation.

### V1.1.0
- Implemented optimized versions of the following functions using MVE instructions:
STM32Ipl_Open , STM32Ipl_Close, STM32Ipl_Erode, STM32Ipl_Dilate, STM32Ipl_Gaussian, STM32Ipl_MedianFilter, STM32Ipl_Diff, STM32Ipl_Binary, STM32Ipl_DrawPolygon

### V1.0.0
- Initial version.
- Based on OpenMV v3.6.9 f79167de4b