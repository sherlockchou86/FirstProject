/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file gstnvdsinfer.h
 * <b>NVIDIA DeepStream GStreamer NvInfer API Specification </b>
 *
 * @b Description: This file specifies the APIs and function definitions for
 * the DeepStream GStreamer NvInfer Plugin.
 */

G_BEGIN_DECLS

#include "nvdsinfer.h"

/**
 * Function definition for the inference raw output generated callback of
 * Gst-NvInfer plugin.
 *
 * The callback function can be registered by setting "raw-output-generated-callback"
 * property on an "nvinfer" element instance. Additionally, a pointer to
 * user data can be set through the "raw-output-generated-userdata" property.
 * This pointer will be passed to the raw output generated callback function
 * through the userdata parameter.
 *
 * Refer to the reference deepstream-app sources for a sample implementation
 * of the callback.
 *
 * @param[in]  buf Pointer to the GstBuffer on whose contents inference has been
 *             executed. The implementation should assume the buffer to be
 *             read-only and should not modify the buffer in any way.
 * @param[in]  network_info Network information for the model specified for the
 *             nvinfer element instance.
 * @param[in]  layers_info Pointer to the array containing information for all
 *             bound layers for the inference engine.
 * @param[in]  num_layers Number of layers bound for the inference engine i.e.
 *             number of elements in the layers_info array.
 * @param[in]  batch_size Number of valid input frames in the batch.
 * @param[in]  user_data Pointer to the user data set through the
 *             "raw-output-generated-userdata" property.
 */
typedef void (* gst_nvinfer_raw_output_generated_callback) (GstBuffer *buf,
    NvDsInferNetworkInfo *network_info,  NvDsInferLayerInfo *layers_info,
    guint num_layers, guint batch_size, gpointer user_data);

G_END_DECLS

