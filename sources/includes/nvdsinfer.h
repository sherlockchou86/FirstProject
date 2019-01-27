/**
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

/**
 * @file nvdsinfer.h
 * <b>NVIDIA DeepStream Inference specifications </b>
 *
 * @b Description: This file specifies the common definitions used in various
 * APIs exposed by the DeepStream GStreamer Inference plugin.
 */

#ifndef _NVDSINFER_H_
#define _NVDSINFER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define NVDSINFER_MAX_DIMS 8

/**
 * Specifies dimensions of a layer.
 */
typedef struct
{
  /** Number of dimesions of the layer.*/
  unsigned int numDims;
  /** Size of the layer in each dimension. */
  unsigned int d[NVDSINFER_MAX_DIMS];
  /** Number of elements in the layer including all dimensions.*/
  unsigned int numElements;
} NvDsInferDims;

/**
 * Specifies dimensions of a layer with 3 dimensions.
 */
typedef struct
{
  /** Channel count of the layer.*/
  unsigned int c;
  /** Height of the layer.*/
  unsigned int h;
  /** Width of the layer.*/
  unsigned int w;
} NvDsInferDimsCHW;

/**
 * Data type of the layers.
 */
typedef enum
{
  /** FP32 format. */
  FLOAT = 0,
  /** FP16 format. */
  HALF = 1,
  /** INT8 format. */
  INT8 = 2,
  /** INT32 format. */
  INT32 = 3
} NvDsInferDataType;

/**
 * Holds information about one layer in the model.
 */
typedef struct
{
  /** Data type of the layer. */
  NvDsInferDataType dataType;
  /** Dimensions of the layer. */
  NvDsInferDims dims;
  /** TensorRT binding index of the layer. */
  int bindingIndex;
  /** Name of the layer. */
  const char* layerName;
  /** Pointer to the buffer for the layer data. */
  void *buffer;
} NvDsInferLayerInfo;

/**
 * Holds information about the model network.
 */
typedef struct
{
  /** Input width for the model. */
  unsigned int width;
  /** Input height for the model. */
  unsigned int height;
} NvDsInferNetworkInfo;

/**
 * Macro to set values on a NvDsInferDimsCHW structure from a NvDsInferDims
 * structure
 */
#define getDimsCHWFromDims(dimsCHW,dims) \
  do { \
    (dimsCHW).c = (dims).d[0]; \
    (dimsCHW).h = (dims).d[1]; \
    (dimsCHW).w = (dims).d[2]; \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif

