/*
 * Copyright (c) 2017-2018 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#include "deepstream_common.h"
#include "deepstream_streammux.h"


// Create bin, add queue and the element, link all elements and ghost pads,
// Set the element properties from the parsed config
gboolean
set_streammux_properties (NvDsStreammuxConfig *config, GstElement *element)
{
  gboolean ret = FALSE;

  g_object_set(G_OBJECT(element), "gpu-id",
               config->gpu_id, NULL);

  g_object_set (G_OBJECT (element), "cuda-memory-type",
        config->cuda_memory_type, NULL);

  g_object_set(G_OBJECT(element), "live-source",
               config->live_source, NULL);

  g_object_set(G_OBJECT(element),
               "batched-push-timeout", config->batched_push_timeout, NULL);

  if (config->batch_size){
      g_object_set(G_OBJECT(element), "batch-size",
          config->batch_size, NULL);
  }

  g_object_set(G_OBJECT(element), "enable-padding",
               config->enable_padding, NULL);

  if (config->pipeline_width && config->pipeline_height) {
    g_object_set(G_OBJECT(element), "width",
                 config->pipeline_width, NULL);
    g_object_set(G_OBJECT(element), "height",
                 config->pipeline_height, NULL);
  }
  return ret;
}
