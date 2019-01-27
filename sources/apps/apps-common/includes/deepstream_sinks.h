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

#ifndef __NVGSTDS_SINKS_H__
#define __NVGSTDS_SINKS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>

typedef enum
{
  NV_DS_SINK_FAKE = 1,
  NV_DS_SINK_RENDER_EGL,
  NV_DS_SINK_ENCODE_FILE,
  NV_DS_SINK_UDPSINK,
} NvDsSinkType;

typedef enum
{
  NV_DS_CONTAINER_MP4 = 1,
  NV_DS_CONTAINER_MKV
} NvDsContainerType;

typedef enum
{
  NV_DS_ENCODER_H264 = 1,
  NV_DS_ENCODER_H265,
  NV_DS_ENCODER_MPEG4
} NvDsEncoderType;

typedef struct
{
  NvDsSinkType type;
  NvDsContainerType container;
  NvDsEncoderType codec;
  gint bitrate;
  gchar *output_file_path;
  guint gpu_id;
  guint rtsp_port;
  guint udp_port;
} NvDsSinkEncoderConfig;

typedef struct
{
  NvDsSinkType type;
  gint width;
  gint height;
  gint sync;
  guint gpu_id;
  guint cuda_memory_type;
} NvDsSinkRenderConfig;

typedef struct
{
  gboolean enable;
  guint source_id;
  NvDsSinkType type;
  NvDsSinkEncoderConfig encoder_config;
  NvDsSinkRenderConfig render_config;
} NvDsSinkSubBinConfig;

typedef struct
{
  GstElement *bin;
  GstElement *queue;
  GstElement *transform;
  GstElement *cap_filter;
  GstElement *enc_caps_filter;
  GstElement *encoder;
  GstElement *mux;
  GstElement *sink;
  GstElement *rtph264pay;
  gulong sink_buffer_probe;
} NvDsSinkBinSubBin;

typedef struct
{
  GstElement *bin;
  GstElement *queue;
  GstElement *tee;

  gint num_bins;
  NvDsSinkBinSubBin sub_bins[MAX_SINK_BINS];
} NvDsSinkBin;

/**
 * Initialize @ref NvDsSinkBin. It creates and adds sink and
 * other elements needed for processing to the bin.
 * It also sets properties mentioned in the configuration file under
 * group @ref CONFIG_GROUP_SINK
 *
 * @param[in] num_sub_bins number of sink elements.
 * @param[in] config_array array of pointers of type @ref NvDsSinkSubBinConfig
 *            parsed from configuration file.
 * @param[in] bin pointer to @ref NvDsSinkBin to be filled.
 * @param[in] index id of source element.
 *
 * @return true if bin created successfully.
 */
gboolean create_sink_bin (guint num_sub_bins,
    NvDsSinkSubBinConfig *config_array, NvDsSinkBin *bin, guint index);

void set_rtsp_udp_port_num (guint rtsp_port_num, guint udp_port_num);

#ifdef __cplusplus
}
#endif

#endif
