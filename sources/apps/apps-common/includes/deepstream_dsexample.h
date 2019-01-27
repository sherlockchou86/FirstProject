/*
 * Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#ifndef _NVGSTDS_DSEXAMPLE_H_
#define _NVGSTDS_DSEXAMPLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>

typedef struct
{
  // Create a bin for the element only if enabled
  gboolean enable;
  // Struct members to store config / properties for the element
  gboolean full_frame;
  gint processing_width;
  gint processing_height;
  guint unique_id;
  guint gpu_id;
  // For nvvidconv
  guint cuda_memory_type;
} NvDsDsExampleConfig;

// Struct to store references to the bin and elements
typedef struct
{
  GstElement *bin;
  GstElement *queue;
  GstElement *pre_conv;
  GstElement *elem_dsexample;
} NvDsDsExampleBin;

// Function to create the bin and set properties
gboolean
create_dsexample_bin (NvDsDsExampleConfig *config, NvDsDsExampleBin *bin);

#ifdef __cplusplus
}
#endif

#endif /* _NVGSTDS_DSEXAMPLE_H_ */
