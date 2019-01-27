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


#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "deepstream_common.h"
#include "deepstream_sinks.h"
#include <gst/rtsp-server/rtsp-server.h>

static guint uid = 0;

GST_DEBUG_CATEGORY_EXTERN (NVDS_APP);

/**
 * Function to create sink bin for Display / Fakesink.
 */
static gboolean
create_render_bin (NvDsSinkRenderConfig *config, NvDsSinkBinSubBin *bin)
{
  gboolean ret = FALSE;
  gchar elem_name[50];
  GstElement *connect_to;
  GstCaps *caps = NULL;

  uid++;

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin%d", uid);
  bin->bin = gst_bin_new (elem_name);
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_sink%d", uid);
  switch (config->type) {
    case NV_DS_SINK_RENDER_EGL:
      GST_CAT_INFO(NVDS_APP,"NVvideo renderer\n");
      bin->sink = gst_element_factory_make (NVDS_ELEM_SINK_EGL, elem_name);
      break;
    case NV_DS_SINK_FAKE:
      bin->sink =
          gst_element_factory_make (NVDS_ELEM_SINK_FAKESINK, elem_name);
      break;
    default:
      return FALSE;
  }

  if (!bin->sink) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->sink), "sync", config->sync, "max-lateness", -1,
      "async", FALSE, NULL);

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_cap_filter%d", uid);
  bin->cap_filter =
    gst_element_factory_make (NVDS_ELEM_CAPS_FILTER, elem_name);
  if (!bin->cap_filter) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }
  gst_bin_add (GST_BIN (bin->bin), bin->cap_filter);

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_transform%d", uid);
  if (config->type == NV_DS_SINK_RENDER_EGL) {
    bin->transform =
        gst_element_factory_make (NVDS_ELEM_VIDEO_CONV, elem_name);
    if (!bin->transform) {
      NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
      goto done;
    }
    gst_bin_add (GST_BIN (bin->bin), bin->transform);

    caps = gst_caps_new_empty_simple ("video/x-raw");

    if (HAS_MEMORY_FEATURES) {
      GstCapsFeatures *feature = NULL;
      feature = gst_caps_features_new (MEMORY_FEATURES, NULL);
      gst_caps_set_features (caps, 0, feature);
    }
    g_object_set (G_OBJECT (bin->cap_filter), "caps", caps, NULL);

    if (HAS_NVGPU) {
      g_object_set (G_OBJECT (bin->transform), "gpu-id", config->gpu_id, NULL);
      g_object_set (G_OBJECT (bin->transform), "cuda-memory-type",
            config->cuda_memory_type, NULL);
    }
  }

  g_snprintf (elem_name, sizeof (elem_name), "render_queue%d", uid);
  bin->queue = gst_element_factory_make (NVDS_ELEM_QUEUE, elem_name);
  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  gst_bin_add_many (GST_BIN (bin->bin), bin->queue, bin->sink, NULL);

  connect_to = bin->sink;

  if (bin->cap_filter) {
    NVGSTDS_LINK_ELEMENT (bin->cap_filter, connect_to);
    connect_to = bin->cap_filter;
  }

  if (bin->transform) {
    NVGSTDS_LINK_ELEMENT (bin->transform, connect_to);
    connect_to = bin->transform;
  }

  NVGSTDS_LINK_ELEMENT (bin->queue, connect_to);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->queue, "sink");

  ret = TRUE;

done:
  if (caps) {
    gst_caps_unref (caps);
  }
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

/**
 * Function to create sink bin to generate encoded output.
 */
static gboolean
create_encode_file_bin (NvDsSinkEncoderConfig *config,
    NvDsSinkBinSubBin *bin)
{
  GstCaps *caps = NULL;
  gboolean ret = FALSE;
  gchar elem_name[50];
  GstElement *sw_conv = NULL;

  uid++;

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin%d", uid);
  bin->bin = gst_bin_new (elem_name);
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_queue%d", uid);
  bin->queue = gst_element_factory_make (NVDS_ELEM_QUEUE, elem_name);
  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_transform%d", uid);
  bin->transform = gst_element_factory_make (NVDS_ELEM_VIDEO_CONV, elem_name);
  if (!bin->transform) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_cap_filter%d", uid);
  bin->cap_filter =
      gst_element_factory_make (NVDS_ELEM_CAPS_FILTER, elem_name);
  if (!bin->cap_filter) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  caps =
      gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING,
      "RGBA", NULL);

  g_object_set (G_OBJECT (bin->cap_filter), "caps", caps, NULL);

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_sw_transform%d", uid);
  sw_conv = gst_element_factory_make ("videoconvert", elem_name);
  if (!sw_conv) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_encoder%d", uid);
  switch (config->codec) {
    case NV_DS_ENCODER_H264:
      bin->encoder = gst_element_factory_make (NVDS_ELEM_ENC_H264, elem_name);
      break;
    case NV_DS_ENCODER_H265:
      bin->encoder = gst_element_factory_make (NVDS_ELEM_ENC_H265, elem_name);
      break;
    case NV_DS_ENCODER_MPEG4:
      bin->encoder = gst_element_factory_make (NVDS_ELEM_ENC_MPEG4, elem_name);
      break;
    default:
      goto done;
  }
  if (!bin->encoder) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->encoder), "bitrate", config->bitrate, NULL);

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_mux%d", uid);
  switch (config->container) {
    case NV_DS_CONTAINER_MP4:
      bin->mux = gst_element_factory_make (NVDS_ELEM_MUX_MP4, elem_name);
      break;
    case NV_DS_CONTAINER_MKV:
      bin->mux = gst_element_factory_make (NVDS_ELEM_MKV, elem_name);
      break;
    default:
      goto done;
  }
  if (!bin->mux) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_sink%d", uid);
  bin->sink = gst_element_factory_make (NVDS_ELEM_SINK_FILE, elem_name);
  if (!bin->sink) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->sink), "location", config->output_file_path,
      "sync", FALSE, "async", FALSE, NULL);
  g_object_set (G_OBJECT (bin->transform), "gpu-id", config->gpu_id, NULL);

  gst_bin_add_many (GST_BIN (bin->bin),
      bin->queue, bin->cap_filter, bin->transform, sw_conv, bin->encoder,
      bin->mux, bin->sink, NULL);

  NVGSTDS_LINK_ELEMENT (bin->queue, bin->transform);
  NVGSTDS_LINK_ELEMENT (bin->transform, bin->cap_filter);
  NVGSTDS_LINK_ELEMENT (bin->cap_filter, sw_conv);
  NVGSTDS_LINK_ELEMENT (sw_conv, bin->encoder);
  NVGSTDS_LINK_ELEMENT (bin->encoder, bin->mux);
  NVGSTDS_LINK_ELEMENT (bin->mux, bin->sink);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->queue, "sink");

  ret = TRUE;

done:
  if (caps) {
    gst_caps_unref (caps);
  }
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

static gboolean
start_rtsp_streaming (guint rtsp_port_num, guint updsink_port_num)
{
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;
  char udpsrc_pipeline[512];

  char port_num_Str[64] = {0};

    sprintf(udpsrc_pipeline,
      "( udpsrc name=pay0 port=%d caps=\"application/x-rtp, media=video, "
      "clock-rate=90000, encoding-name=H264, payload=96, "
      "profile-level-id=428014\" ) ",
      updsink_port_num);


  sprintf(port_num_Str, "%d", rtsp_port_num);

  server = gst_rtsp_server_new ();
  g_object_set (server, "service", port_num_Str, NULL);

  mounts = gst_rtsp_server_get_mount_points (server);

  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory, udpsrc_pipeline);

  gst_rtsp_mount_points_add_factory (mounts, "/ds-test", factory);

  g_object_unref (mounts);

  gst_rtsp_server_attach (server, NULL);

  g_print ("\n *** DeepStream: Launched RTSP Streaming at rtsp://localhost:%d/ds-test ***\n\n", rtsp_port_num);

  return TRUE;
}

static gboolean
create_udpsink_bin (NvDsSinkEncoderConfig * config,
    NvDsSinkBinSubBin * bin)
{
  GstCaps *caps = NULL;
  GstCaps *enc_caps = NULL;
  gboolean ret = FALSE;
  gchar elem_name[50];
  GstElement *sw_conv = NULL;

  //guint rtsp_port_num = g_rtsp_port_num++;
  uid++;

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin%d", uid);
  bin->bin = gst_bin_new (elem_name);
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_queue%d", uid);
  bin->queue = gst_element_factory_make (NVDS_ELEM_QUEUE, elem_name);
  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_transform%d", uid);
  bin->transform = gst_element_factory_make (NVDS_ELEM_VIDEO_CONV, elem_name);
  if (!bin->transform) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_cap_filter%d", uid);
  bin->cap_filter =
      gst_element_factory_make (NVDS_ELEM_CAPS_FILTER, elem_name);
  if (!bin->cap_filter) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  caps =
      gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING,
                           "RGBA", NULL);

  g_object_set (G_OBJECT (bin->cap_filter), "caps", caps, NULL);

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_sw_transform%d", uid);
  sw_conv = gst_element_factory_make ("videoconvert", elem_name);
  if (!sw_conv) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_encoder%d", uid);
  switch (config->codec) {
    case NV_DS_ENCODER_H264:
      bin->encoder = gst_element_factory_make (NVDS_ELEM_ENC_H264, elem_name);
      break;
    default:
      goto done;
  }
  if (!bin->encoder) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->encoder), "bitrate", config->bitrate, NULL);
  g_object_set (G_OBJECT (bin->encoder), "speed-preset", 3, NULL);
  g_object_set (G_OBJECT (bin->encoder), "intra-refresh", TRUE, NULL);
  g_object_set (G_OBJECT (bin->encoder), "ref", 1, NULL);
  g_object_set (G_OBJECT (bin->encoder), "key-int-max", 1, NULL);
  g_object_set (G_OBJECT (bin->transform), "gpu-id", config->gpu_id, NULL);

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_enc_caps_filter%d", uid);
  bin->enc_caps_filter =
      gst_element_factory_make (NVDS_ELEM_CAPS_FILTER, elem_name);
  if (!bin->enc_caps_filter) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  enc_caps = gst_caps_new_simple ("video/x-h264", "profile", G_TYPE_STRING,"baseline", NULL);
  g_object_set (G_OBJECT (bin->enc_caps_filter), "caps", enc_caps, NULL);

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_rtph264pay%d", uid);
  bin->rtph264pay = gst_element_factory_make ("rtph264pay", elem_name);
  if (!bin->rtph264pay) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_udpsink%d", uid);
  bin->sink = gst_element_factory_make ("udpsink", elem_name);
  if (!bin->sink) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->sink), "host", "224.224.255.255", "port",
      config->udp_port, "async", FALSE, NULL);

  gst_bin_add_many (GST_BIN (bin->bin),
                    bin->queue, bin->cap_filter, bin->transform, sw_conv, bin->encoder,
                    bin->enc_caps_filter, bin->rtph264pay, bin->sink, NULL);

  NVGSTDS_LINK_ELEMENT (bin->queue, bin->transform);
  NVGSTDS_LINK_ELEMENT (bin->transform, bin->cap_filter);


  NVGSTDS_LINK_ELEMENT (bin->cap_filter, sw_conv);
  NVGSTDS_LINK_ELEMENT (sw_conv, bin->encoder);

  NVGSTDS_LINK_ELEMENT (bin->encoder, bin->enc_caps_filter);
  NVGSTDS_LINK_ELEMENT (bin->enc_caps_filter, bin->rtph264pay);
  NVGSTDS_LINK_ELEMENT (bin->rtph264pay, bin->sink);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->queue, "sink");

  ret = TRUE;

  ret = start_rtsp_streaming (config->rtsp_port, config->udp_port);
  if (ret != TRUE)
  {
    g_print ("%s: start_rtsp_straming function failed\n", __func__);
  }

done:
  if (caps) {
    gst_caps_unref (caps);
  }
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

gboolean
create_sink_bin (guint num_sub_bins, NvDsSinkSubBinConfig *config_array,
    NvDsSinkBin *bin, guint index)
{
  gboolean ret = FALSE;
  guint i;

  bin->bin = gst_bin_new ("sink_bin");
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create element 'sink_bin'");
    goto done;
  }

  bin->queue = gst_element_factory_make (NVDS_ELEM_QUEUE, "sink_bin_queue");
  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create element 'sink_bin_queue'");
    goto done;
  }

  gst_bin_add (GST_BIN (bin->bin), bin->queue);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->queue, "sink");

  bin->tee = gst_element_factory_make (NVDS_ELEM_TEE, "sink_bin_tee");
  if (!bin->tee) {
    NVGSTDS_ERR_MSG_V ("Failed to create element 'sink_bin_tee'");
    goto done;
  }

  gst_bin_add (GST_BIN (bin->bin), bin->tee);

  NVGSTDS_LINK_ELEMENT (bin->queue, bin->tee);

  for (i = 0; i < num_sub_bins; i++) {
    if (!config_array[i].enable) {
      continue;
    }
    if (config_array[i].source_id != (gint) index) {
      continue;
    }
    switch (config_array[i].type) {
      case NV_DS_SINK_RENDER_EGL:
      case NV_DS_SINK_FAKE:
        config_array[i].render_config.type = config_array[i].type;
        if (!create_render_bin (&config_array[i].render_config,
                                &bin->sub_bins[i]))
          goto done;
        break;
      case NV_DS_SINK_ENCODE_FILE:
        if (!create_encode_file_bin (&config_array[i].encoder_config,
                &bin->sub_bins[i]))
          goto done;
        break;
      case NV_DS_SINK_UDPSINK:
        if (!create_udpsink_bin (&config_array[i].encoder_config,
                &bin->sub_bins[i]))
          goto done;
        break;
      default:
        goto done;
    }
    gst_bin_add (GST_BIN (bin->bin), bin->sub_bins[i].bin);

    if (!link_element_to_tee_src_pad (bin->tee, bin->sub_bins[i].bin)) {
      goto done;
    }
    bin->num_bins++;
  }

  if (bin->num_bins == 0) {
    NvDsSinkRenderConfig config;
    config.type = NV_DS_SINK_FAKE;
    if (!create_render_bin (&config, &bin->sub_bins[0]))
      goto done;
    gst_bin_add (GST_BIN (bin->bin), bin->sub_bins[0].bin);
    if (!link_element_to_tee_src_pad (bin->tee, bin->sub_bins[0].bin)) {
      goto done;
    }
    bin->num_bins = 1;
  }

  ret = TRUE;
done:
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}
