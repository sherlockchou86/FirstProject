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

#ifndef __NVGSTDS_TRACKER_H__
#define __NVGSTDS_TRACKER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>

typedef struct
{
  gboolean enable;
  gint width;
  gint height;
  guint gpu_id;
  guint tracker_algo;
  gfloat iou_threshold;
  guint tracking_surf_type;
} NvDsTrackerConfig;

typedef struct
{
  GstElement *bin;
  GstElement *tracker;
} NvDsTrackerBin;

/**
 * Initialize @ref NvDsTrackerBin. It creates and adds tracker and
 * other elements needed for processing to the bin.
 * It also sets properties mentioned in the configuration file under
 * group @ref CONFIG_GROUP_TRACKER
 *
 * @param[in] config pointer of type @ref NvDsTrackerConfig
 *            parsed from configuration file.
 * @param[in] bin pointer of type @ref NvDsTrackerBin to be filled.
 *
 * @return true if bin created successfully.
 */
gboolean
create_tracking_bin (NvDsTrackerConfig * config, NvDsTrackerBin * bin);

#ifdef __cplusplus
}
#endif

#endif
