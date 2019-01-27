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

#include "deepstream_common.h"
#include "deepstream_tiled_display.h"

gboolean
create_tiled_display_bin (NvDsTiledDisplayConfig *config, NvDsTiledDisplayBin *bin)
{
  gboolean ret = FALSE;

  bin->bin = gst_bin_new ("tiled_display_bin");
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'tiled_display_bin'");
    goto done;
  }

  bin->queue = gst_element_factory_make (NVDS_ELEM_QUEUE, "tiled_display_queue");
  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'tiled_display_queue'");
    goto done;
  }

  if (config->enable == 1)
    bin->tiler = gst_element_factory_make (NVDS_ELEM_TILER, "tiled_display_tiler");
  else
    bin->tiler = gst_element_factory_make ("identity", "tiled_display_identity");

  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'tiled_display_queue'");
    goto done;
  }

  if (config->width)
    g_object_set (G_OBJECT (bin->tiler), "width", config->width, NULL);

  if (config->height)
    g_object_set (G_OBJECT (bin->tiler), "height", config->height, NULL);

  if (config->rows)
    g_object_set (G_OBJECT (bin->tiler), "rows", config->rows, NULL);

  if (config->columns)
    g_object_set (G_OBJECT (bin->tiler), "columns", config->columns, NULL);

  g_object_set (G_OBJECT (bin->tiler), "gpu-id", config->gpu_id,
       "cuda-memory-type", config->cuda_memory_type, NULL);

  gst_bin_add_many (GST_BIN (bin->bin), bin->queue, bin->tiler, NULL);

  NVGSTDS_LINK_ELEMENT (bin->queue, bin->tiler);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->queue, "sink");

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->tiler, "src");

  ret = TRUE;
done:
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}
