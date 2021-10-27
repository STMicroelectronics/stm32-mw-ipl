/**
 ******************************************************************************
 * @file   stm32ipl_object_det.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - object detection module
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

#include "stm32ipl.h"
#include "stm32ipl_imlib_int.h"

#ifdef STM32IPL_ENABLE_OBJECT_DETECTION

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loads a cascade from memory.
 * @param cascade 	Pointer to the cascade; if it is not valid, an error is returned.
 * @param memory  	Pointer to the memory containing the cascade; if it is not valid, an error is returned.
 * @return			stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_LoadCascadeFromMemory(cascade_t *cascade, const uint8_t *memory)
{
	int i;
	uint8_t *mem = (uint8_t*)memory;

	/* Check arguments. */
	if (!cascade || !memory)
		return stm32ipl_err_InvalidParameter;

	memset(cascade, 0, sizeof(cascade_t));

	/* read detection window size */
	// read_data(&fp, &cascade->window, sizeof(cascade->window));
	memcpy(&cascade->window, mem, sizeof(cascade->window));
	mem += sizeof(cascade->window);

	/* read num stages */
	// read_data(&fp, &cascade->n_stages, sizeof(cascade->n_stages));
	memcpy(&cascade->n_stages, mem, sizeof(cascade->n_stages));
	mem += sizeof(cascade->n_stages);

	/* read num features in each stages */
	// read_data(&fp, cascade->stages_array, sizeof(uint8_t) * cascade->n_stages);
	cascade->stages_array = mem;
	mem += (sizeof(uint8_t) * cascade->n_stages);

	/* sum num of features in each stages*/
	for (i = 0, cascade->n_features = 0; i < cascade->n_stages; i++) {
		cascade->n_features += cascade->stages_array[i];
	}

	/* read stages thresholds */
	// read_data(&fp, cascade->stages_thresh_array, sizeof(int16_t)*cascade->n_stages);
	cascade->stages_thresh_array = (int16_t*)mem;
	mem += (sizeof(int16_t) * cascade->n_stages);

	/* read features thresholds */
	// read_data(&fp, cascade->tree_thresh_array, sizeof(*cascade->tree_thresh_array)*cascade->n_features);
	cascade->tree_thresh_array = (int16_t*)mem;
	mem += (sizeof(*cascade->tree_thresh_array) * cascade->n_features);

	/* read alpha 1 */
	// read_data(&fp, cascade->alpha1_array, sizeof(*cascade->alpha1_array)*cascade->n_features);
	cascade->alpha1_array = (int16_t*)mem;
	mem += (sizeof(*cascade->alpha1_array) * cascade->n_features);

	/* read alpha 2 */
	// read_data(&fp, cascade->alpha2_array, sizeof(*cascade->alpha2_array)*cascade->n_features);
	cascade->alpha2_array = (int16_t*)mem;
	mem += (sizeof(*cascade->alpha2_array) * cascade->n_features);

	/* read num rectangles per feature*/
	// read_data(&fp, cascade->num_rectangles_array, sizeof(*cascade->num_rectangles_array)*cascade->n_features);
	cascade->num_rectangles_array = (int8_t*)mem;
	mem += (sizeof(*cascade->num_rectangles_array) * cascade->n_features);

	/* sum num of recatngles per feature*/
	for (i = 0, cascade->n_rectangles = 0; i < cascade->n_features; i++) {
		cascade->n_rectangles += cascade->num_rectangles_array[i];
	}

	/* read rectangles weights */
	// read_data(&fp, cascade->weights_array, sizeof(*cascade->weights_array)*cascade->n_rectangles);
	cascade->weights_array = (int8_t*)mem;
	mem += (sizeof(*cascade->weights_array) * cascade->n_rectangles);

	/* read rectangles num rectangles * 4 points */
	//read_data(&fp, cascade->rectangles_array, sizeof(*cascade->rectangles_array)*cascade->n_rectangles *4);
	cascade->rectangles_array = (int8_t*)mem;

	return stm32ipl_err_Ok;
}

#ifdef STM32IPL_ENABLE_FRONTAL_FACE_CASCADE
/**
 * @brief Loads the frontal face cascade.
 * @param cascade 	Pointer to the cascade; if it is not valid, an error is returned.
 * @return			stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_LoadFaceCascade(cascade_t *cascade)
{
	STM32IPL_CHECK_VALID_PTR_ARG(cascade)

	imlib_load_cascade(cascade, "frontalface");
	return stm32ipl_err_Ok;
}
#endif /* STM32IPL_ENABLE_FRONTAL_FACE_CASCADE */

#ifdef STM32IPL_ENABLE_EYE_CASCADE
/**
 * @brief Loads the eye cascade.
 * @param cascade 	Pointer to the cascade; if it is not valid, an error is returned.
 * @return			stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_LoadEyeCascade(cascade_t *cascade)
{
	STM32IPL_CHECK_VALID_PTR_ARG(cascade)

	imlib_load_cascade(cascade, "eye");
	return stm32ipl_err_Ok;
}
#endif /* STM32IPL_ENABLE_EYE_CASCADE */

/**
 * @brief Detects objects, described by the given cascade. The detected object are stored in an array_t
 * structure containing the bounding boxes (rectangle_t) one for each object detected; the caller is
 * responsible to release the array. The supported formats are Grayscale, RGB565, RGB888.
 * @param img			Image; if it is not valid, an error is returned.
 * @param out			Pointer to pointer to the array structure that will contain the detected objects.
 * It must point to a valid, but empty structure. It MUST be released by the caller.
 * @param roi			Optional region of interest of the source image where the functions operates;
 * when defined, it must be contained in the source image and have positive dimensions, otherwise
 * an error is returned; when not defined, the whole image is considered.
 * @param cascade		Pointer to a cascade (must be already loaded with specific loading function).
 * @param scaleFactor	Tune the capability to detect objects at different scale (must be > 1.0f).
 * @param threshold		Tune the detection rate against the false positive rate (0.0f - 1.0f).
 * @return				stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_DetectObject(const image_t *img, array_t **out, const rectangle_t *roi, cascade_t *cascade,
		float scaleFactor, float threshold)
{
	rectangle_t realRoi;

	STM32IPL_CHECK_VALID_IMAGE(img)
	STM32IPL_CHECK_FORMAT(img, (stm32ipl_if_grayscale | stm32ipl_if_rgb565 | stm32ipl_if_rgb888))
	STM32IPL_CHECK_VALID_PTR_ARG(cascade)
	STM32IPL_GET_REAL_ROI(img, roi, &realRoi)

	cascade->scale_factor = scaleFactor;
	cascade->threshold = threshold;

	*out = imlib_detect_objects((image_t*)img, cascade, &realRoi);

	return stm32ipl_err_Ok;
}

#ifdef __cplusplus
}
#endif

#endif /* STM32IPL_ENABLE_OBJECT_DETECTION */
