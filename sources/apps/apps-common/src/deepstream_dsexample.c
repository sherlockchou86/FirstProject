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
#include "deepstream_dsexample.h"


// Create bin, add queue and the element, link all elements and ghost pads,
// Set the element properties from the parsed config
gboolean
create_dsexample_bin (NvDsDsExampleConfig *config, NvDsDsExampleBin *bin)
{
  gboolean ret = FALSE;

  bin->bin = gst_bin_new ("dsexample_bin");
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'dsexample_bin'");
    goto done;
  }

  bin->queue =
      gst_element_factory_make (NVDS_ELEM_QUEUE, "dsexample_queue");
  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'dsexample_queue'");
    goto done;
  }

  bin->elem_dsexample =
      gst_element_factory_make (NVDS_ELEM_DSEXAMPLE_ELEMENT, "dsexample0");
  if (!bin->elem_dsexample) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'dsexample0'");
    goto done;
  }

  bin->pre_conv = gst_element_factory_make (NVDS_ELEM_VIDEO_CONV, "dsexample_conv0");
  if (!bin->pre_conv) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'dsexample_conv0'");
    goto done;
  }

  gst_bin_add_many (GST_BIN (bin->bin), bin->queue,
      bin->pre_conv, bin->elem_dsexample, NULL);

  NVGSTDS_LINK_ELEMENT (bin->queue, bin->pre_conv);
  NVGSTDS_LINK_ELEMENT (bin->pre_conv, bin->elem_dsexample);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->queue, "sink");

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->elem_dsexample, "src");

  g_object_set (G_OBJECT (bin->elem_dsexample),
      "full-frame", config->full_frame,
      "processing-width", config->processing_width,
      "processing-height", config->processing_height,
      "unique-id", config->unique_id,
      "gpu-id", config->gpu_id, NULL);

 g_object_set (G_OBJECT (bin->pre_conv), "gpu-id", config->gpu_id, NULL);

 g_object_set (G_OBJECT (bin->pre_conv), "cuda-memory-type",
     config->cuda_memory_type, NULL);

 ret = TRUE;

done:
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }

  return ret;
}
