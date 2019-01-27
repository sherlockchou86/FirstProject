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

#ifndef __NVGSTDS_SOURCES_H__
#define __NVGSTDS_SOURCES_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>
#include "deepstream_dewarper.h"

typedef enum
{
  NV_DS_SOURCE_CAMERA_V4L2 = 1,
  NV_DS_SOURCE_URI,
  NV_DS_SOURCE_URI_MULTIPLE,
  NV_DS_SOURCE_RTSP
} NvDsSourceType;

typedef struct
{
  NvDsSourceType type;
  gboolean enable;
  gboolean live_source;
  gboolean Intra_decode;
  gint source_width;
  gint source_height;
  gint source_fps_n;
  gint source_fps_d;
  gint camera_v4l2_dev_node;
  gchar *uri;
  gint latency;
  guint num_sources;
  guint gpu_id;
  guint camera_id;
  guint select_rtp_protocol;
  guint num_decode_surfaces;
  guint cuda_memory_type;
  NvDsDewarperConfig dewarper_config;
} NvDsSourceConfig;

typedef struct
{
  GstElement *bin;
  GstElement *src_elem;
  GstElement *cap_filter;
  GstElement *depay;
  GstElement *enc_que;
  GstElement *dec_que;
  GstElement *decodebin;
  GstElement *enc_filter;
  GstElement *encbin_que;
  GstElement *tee;
  GstElement *fakesink_queue;
  GstElement *fakesink;
  gboolean do_record;
  guint64 pre_event_rec;
  GMutex bin_lock;
  guint bin_id;
  gulong src_buffer_probe;
  gpointer bbox_meta;
  GstBuffer *inbuf;
  gchar *location;
  gchar *file;
  gchar *direction;
  gint latency;
  gboolean got_key_frame;
  gboolean eos_done;
  gboolean reset_done;
  gboolean live_source;
  gboolean reconfiguring;
  NvDsDewarperBin dewarper_bin;
  gulong probe_id;
  guint64 accumulated_base;
} NvDsSrcBin;

typedef struct
{
  GstElement *bin;
  GstElement *streammux;
  GThread *reset_thread;
  NvDsSrcBin sub_bins[MAX_SOURCE_BINS];
  guint num_bins;
  guint num_fr_on;
  gboolean live_source;
} NvDsSrcParentBin;


gboolean create_source_bin (NvDsSourceConfig *config, NvDsSrcBin *bin);

/**
 * Initialize @ref NvDsSrcParentBin. It creates and adds source and
 * other elements needed for processing to the bin.
 * It also sets properties mentioned in the configuration file under
 * group @ref CONFIG_GROUP_SOURCE
 *
 * @param[in] num_sub_bins number of source elements.
 * @param[in] configs array of pointers of type @ref NvDsSourceConfig
 *            parsed from configuration file.
 * @param[in] bin pointer to @ref NvDsSrcParentBin to be filled.
 *
 * @return true if bin created successfully.
 */
gboolean
create_multi_source_bin (guint num_sub_bins, NvDsSourceConfig *configs,
                         NvDsSrcParentBin *bin);

gboolean reset_source_pipeline (gpointer data);
gboolean set_source_to_playing (gpointer data);
gpointer reset_encodebin (gpointer data);
#ifdef __cplusplus
}
#endif

#endif
