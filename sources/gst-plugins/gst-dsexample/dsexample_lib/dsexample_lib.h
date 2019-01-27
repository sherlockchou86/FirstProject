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

#ifndef __DSEXAMPLE_LIB__
#define __DSEXAMPLE_LIB__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DsExampleCtx DsExampleCtx;

// Init parameters structure as input, required for instantiating dsexample_lib
typedef struct
{
  // Width at which frame/object will be scaled
  int processingWidth;
  // height at which frame/object will be scaled
  int processingHeight;
  // Flag to indicate whether operating on crops of full frame
  int fullFrame;
} DsExampleInitParams;

// Detected/Labelled object structure, stores bounding box info along with label
typedef struct
{
  int left;
  int top;
  int width;
  int height;
  char label[64];
} DsExampleObject;

// Output data returned after processing
typedef struct
{
  int numObjects;
  DsExampleObject object[4];
} DsExampleOutput;

// Initialize library context
DsExampleCtx * DsExampleCtxInit (DsExampleInitParams *init_params);

// Dequeue processed output
DsExampleOutput *DsExampleProcess (DsExampleCtx *ctx, unsigned char *data);

// Deinitialize library context
void DsExampleCtxDeinit (DsExampleCtx *ctx);

#ifdef __cplusplus
}
#endif

#endif
