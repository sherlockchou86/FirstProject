/*
 * Copyright (c) 2016-2018 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#ifndef __NVGSTDS_DEWARPER_H__
#define __NVGSTDS_DEWARPER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>

typedef struct
{
  GstElement *bin;
  GstElement *queue;
  GstElement *src_queue;
  GstElement *nvvidconv;
  GstElement *cap_filter;
  GstElement *dewarper_caps_filter;
  GstElement *nvdewarper;
} NvDsDewarperBin;

typedef struct
{
  gboolean enable;
  guint gpu_id;
  guint num_out_buffers;
  guint dewarper_dump_frames;
  gchar *config_file;
  guint cuda_memory_type;
} NvDsDewarperConfig;

gboolean create_dewarper_bin (NvDsDewarperConfig * config, NvDsDewarperBin * bin);

#ifdef __cplusplus
}
#endif

#endif
