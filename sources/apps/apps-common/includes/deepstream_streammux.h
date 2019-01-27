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

#ifndef _NVGSTDS_STREAMMUX_H_
#define _NVGSTDS_STREAMMUX_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>

typedef struct
{
  // Struct members to store config / properties for the element
  gint pipeline_width;
  gint pipeline_height;
  gint batch_size;
  gint batched_push_timeout;
  guint gpu_id;
  guint cuda_memory_type;
  gboolean live_source;
  gboolean enable_padding;
  gboolean is_parsed;

} NvDsStreammuxConfig;

// Function to create the bin and set properties
gboolean
set_streammux_properties (NvDsStreammuxConfig *config, GstElement *streammux);

#ifdef __cplusplus
}
#endif

#endif /* _NVGSTDS_DSEXAMPLE_H_ */
