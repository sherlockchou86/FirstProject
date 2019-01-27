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

#ifndef __NVGSTDS_TILED_DISPLAY_H__
#define __NVGSTDS_TILED_DISPLAY_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>
#include "nvosd.h"

typedef struct
{
  GstElement *bin;
  GstElement *queue;
  GstElement *tiler;
} NvDsTiledDisplayBin;

typedef struct
{
  gboolean enable;
  guint rows;
  guint columns;
  guint width;
  guint height;
  guint gpu_id;
  guint cuda_memory_type;
} NvDsTiledDisplayConfig;

/**
 * Initialize @ref NvDsTiledDisplayBin. It creates and adds tiling and
 * other elements needed for processing to the bin.
 * It also sets properties mentioned in the configuration file under
 * group @ref CONFIG_GROUP_TILED_DISPLAY
 *
 * @param[in] config pointer of type @ref NvDsTiledDisplayConfig
 *            parsed from configuration file.
 * @param[in] bin pointer to @ref NvDsTiledDisplayBin to be filled.
 *
 * @return true if bin created successfully.
 */
gboolean
create_tiled_display_bin (NvDsTiledDisplayConfig * config,
                          NvDsTiledDisplayBin * bin);

#ifdef __cplusplus
}
#endif

#endif
