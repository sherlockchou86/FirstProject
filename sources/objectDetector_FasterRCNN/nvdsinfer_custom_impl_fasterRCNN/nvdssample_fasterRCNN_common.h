/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __NVDS_SAMPLE_FASTERRCNN_COMMON_H__
#define __NVDS_SAMPLE_FASTERRCNN_COMMON_H__

const int nmsMaxOut = 300;
const int poolingH = 7;
const int poolingW = 7;
const int featureStride = 16;
const int preNmsTop = 6000;
const int anchorsRatioCount = 3;
const int anchorsScaleCount = 3;
const float iouThreshold = 0.7f;
const float minBoxSize = 16;
const float spatialScale = 0.0625f;
const float anchorsRatios[anchorsRatioCount] = {0.5f, 1.0f, 2.0f};
const float anchorsScales[anchorsScaleCount] = {8.0f, 16.0f, 32.0f};

#endif
