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

#include <string.h>

#include "gstnvdsmeta.h"
#include "deepstream_common.h"
#include "deepstream_sources.h"
#include "deepstream_dewarper.h"

#define SRC_CONFIG_KEY "src_config"

GST_DEBUG_CATEGORY_EXTERN (NVDS_APP);
GST_DEBUG_CATEGORY_EXTERN (APP_CFG_PARSER_CAT);

static gboolean
set_camera_v4l2_params (NvDsSourceConfig *config, NvDsSrcBin *bin)
{
  gchar device[64];

  g_snprintf (device, sizeof (device), "/dev/video%d",
      config->camera_v4l2_dev_node);
  g_object_set (G_OBJECT (bin->src_elem), "device", device, NULL);

  GST_CAT_DEBUG (NVDS_APP, "Setting v4l2 camera params successful");

  return TRUE;
}

static gboolean
create_camera_source_bin (NvDsSourceConfig *config, NvDsSrcBin *bin)
{
  gboolean ret = FALSE;
  GstCaps *caps = NULL, *convertCaps = NULL;
  GstElement *convert, *convertFilter = NULL;
  GstElement *nvconvert, *nvconvertFilter;
  GstCaps *nvconvertCaps;

  switch (config->type) {
    case NV_DS_SOURCE_CAMERA_V4L2:
      bin->src_elem =
          gst_element_factory_make (NVDS_ELEM_SRC_CAMERA_V4L2, "src_elem");
      break;
    default:
      NVGSTDS_ERR_MSG_V ("Unsupported source type");
      goto done;
  }

  if (!bin->src_elem) {
    NVGSTDS_ERR_MSG_V ("Could not create 'src_elem'");
    goto done;
  }

  bin->cap_filter =
      gst_element_factory_make (NVDS_ELEM_CAPS_FILTER, "src_cap_filter");
  if (!bin->cap_filter) {
    NVGSTDS_ERR_MSG_V ("Could not create 'src_cap_filter'");
    goto done;
  }

  caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "I420",
      "width", G_TYPE_INT, config->source_width, "height", G_TYPE_INT,
      config->source_height, "framerate", GST_TYPE_FRACTION,
      config->source_fps_n, config->source_fps_d, NULL);

  g_object_set (G_OBJECT (bin->cap_filter), "caps", caps, NULL);

  convert = gst_element_factory_make ("videoconvert", "v4l2src_video_convert");
  if (!convert) {
    NVGSTDS_ERR_MSG_V ("Could not create 'v4l2src_convert'");
    goto done;
  }

  convertFilter = gst_element_factory_make (NVDS_ELEM_CAPS_FILTER,
                                            "src_convert_filter");
  if (!convertFilter) {
    NVGSTDS_ERR_MSG_V ("Could not create 'src_convert_filter'");
    goto done;
  }

  convertCaps = gst_caps_copy (caps);
  gst_caps_set_simple (convertCaps, "format", G_TYPE_STRING, "NV12", NULL);
  g_object_set (G_OBJECT (convertFilter), "caps", convertCaps, NULL);


  nvconvert = gst_element_factory_make (NVDS_ELEM_VIDEO_CONV,
                                        "v4l2src_video_convert2");
  if (!nvconvert) {
    NVGSTDS_ERR_MSG_V ("Could not create 'v4l2src_convert'");
    goto done;
  }

  nvconvertFilter = gst_element_factory_make (NVDS_ELEM_CAPS_FILTER,
                                              "src_convert_filter2");
  if (!nvconvertFilter) {
    NVGSTDS_ERR_MSG_V ("Could not create 'src_convert_filter'");
    goto done;
  }

  nvconvertCaps = gst_caps_copy (convertCaps);

  GstCapsFeatures *feature = NULL;
  feature = gst_caps_features_new (MEMORY_FEATURES, NULL);
  gst_caps_set_features (nvconvertCaps, 0, feature);
  g_object_set (G_OBJECT (nvconvertFilter), "caps", nvconvertCaps, NULL);
  gst_caps_unref (nvconvertCaps);

  gst_bin_add_many (GST_BIN (bin->bin), bin->src_elem, bin->cap_filter,
                    convert, convertFilter, nvconvert, nvconvertFilter, NULL);

  NVGSTDS_LINK_ELEMENT (bin->src_elem, bin->cap_filter);
  NVGSTDS_LINK_ELEMENT (bin->cap_filter, convert);
  NVGSTDS_LINK_ELEMENT (convert, convertFilter);
  NVGSTDS_LINK_ELEMENT (convertFilter, nvconvert);
  NVGSTDS_LINK_ELEMENT (nvconvert, nvconvertFilter);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, nvconvertFilter, "src");

  switch (config->type) {
    case NV_DS_SOURCE_CAMERA_V4L2:
      if (!set_camera_v4l2_params (config, bin)) {
        NVGSTDS_ERR_MSG_V ("Could not set V4L2 camera properties");
        goto done;
      }
      break;
    default:
      NVGSTDS_ERR_MSG_V ("Unsupported source type");
      goto done;
  }

  ret = TRUE;

  GST_CAT_DEBUG (NVDS_APP, "Created camera source bin successfully");

done:
  if (caps)
    gst_caps_unref (caps);

  if (convertCaps)
    gst_caps_unref (convertCaps);

  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

static void
cb_newpad (GstElement *decodebin, GstPad *pad, gpointer data)
{
  GstCaps *caps = gst_pad_query_caps (pad, NULL);
  const GstStructure *str = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (str);

  if (!strncmp (name, "video", 5)) {
    NvDsSrcBin *bin = (NvDsSrcBin *) data;
    GstPad *sinkpad = gst_element_get_static_pad (bin->tee, "sink");
    if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK) {

      NVGSTDS_ERR_MSG_V ("Failed to link decodebin to pipeline");
    } else {
      NvDsSourceConfig *config =
          (NvDsSourceConfig *) g_object_get_data (G_OBJECT (bin->cap_filter),
          SRC_CONFIG_KEY);

      gst_structure_get_int (str, "width", &config->source_width);
      gst_structure_get_int (str, "height", &config->source_height);
      gst_structure_get_fraction (str, "framerate", &config->source_fps_n,
          &config->source_fps_d);

      GST_CAT_DEBUG (NVDS_APP, "Decodebin linked to pipeline");
    }
    gst_object_unref (sinkpad);
  }
}

static void
cb_sourcesetup (GstElement *object, GstElement *arg0, gpointer data)
{
  NvDsSrcBin *bin = (NvDsSrcBin *) data;
  if(g_object_class_find_property(G_OBJECT_GET_CLASS(arg0),"latency"))
  {
      g_print("cb_sourcesetup set %d latency\n",bin->latency);
      g_object_set (G_OBJECT (arg0), "latency", bin->latency, NULL);
  }
}

void
decodebin_child_added (GstChildProxy *child_proxy, GObject *object,
                       gchar *name, gpointer user_data)
{
  NvDsSourceConfig *config = (NvDsSourceConfig *) user_data;
  if (g_strrstr (name, "decodebin") == name) {
    g_signal_connect (G_OBJECT (object), "child-added",
        G_CALLBACK (decodebin_child_added), user_data);
  }
  if (g_strrstr (name, "nvcuvid") == name) {
    g_object_set (object, "gpu-id", config->gpu_id, NULL);

    g_object_set (G_OBJECT (object), "cuda-memory-type",
          config->cuda_memory_type, NULL);

    g_object_set (object, "source-id", config->camera_id, NULL);
    g_object_set (object, "num-decode-surfaces", config->num_decode_surfaces, NULL);
    if (config->Intra_decode)
      g_object_set (object, "Intra-decode", config->Intra_decode, NULL);
  }
}

static void
cb_newpad2 (GstElement * decodebin, GstPad * pad, gpointer data)
{
  GstCaps *caps = gst_pad_query_caps (pad, NULL);
  const GstStructure *str = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (str);

  if (!strncmp (name, "video", 5)) {
    NvDsSrcBin *bin = (NvDsSrcBin *) data;
//    GstPad *sinkpad = gst_element_get_static_pad (bin->cap_filter, "sink");
    GstPad *sinkpad = gst_element_get_static_pad (bin->cap_filter, "sink");
    if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK) {

      NVGSTDS_ERR_MSG_V ("Failed to link decodebin to pipeline");
    } else {
      NvDsSourceConfig *config =
          (NvDsSourceConfig *) g_object_get_data (G_OBJECT (bin->cap_filter),
          SRC_CONFIG_KEY);

      gst_structure_get_int (str, "width", &config->source_width);
      gst_structure_get_int (str, "height", &config->source_height);
      gst_structure_get_fraction (str, "framerate", &config->source_fps_n,
          &config->source_fps_d);

      GST_CAT_DEBUG (NVDS_APP, "Decodebin linked to pipeline");
    }
    gst_object_unref (sinkpad);
  }
}


static void
cb_newpad3 (GstElement * decodebin, GstPad * pad, gpointer data)
{
  GstCaps *caps = gst_pad_query_caps (pad, NULL);
  const GstStructure *str = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (str);

  if (g_strrstr (name, "x-rtp")) {
    NvDsSrcBin *bin = (NvDsSrcBin *) data;
    GstPad *sinkpad = gst_element_get_static_pad (bin->depay, "sink");
    if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK) {

      NVGSTDS_ERR_MSG_V ("Failed to link depay loader to rtsp src");
    }
    gst_object_unref (sinkpad);
  }
}

static gboolean
create_rtsp_src_bin (NvDsSourceConfig *config, NvDsSrcBin *bin)
{
  gboolean ret = FALSE;
  gchar elem_name[50];

  bin->latency = config->latency;

  g_snprintf (elem_name, sizeof (elem_name), "src_elem%d", bin->bin_id);
  bin->src_elem = gst_element_factory_make ("rtspsrc", elem_name);
  if (!bin->src_elem) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->src_elem), "location", config->uri, NULL);
  g_object_set (G_OBJECT (bin->src_elem), "latency", config->latency, NULL);
  g_object_set (G_OBJECT (bin->src_elem), "drop-on-latency", TRUE, NULL);
  // 0x4 for TCP and 0x7 for All (UDP/UDP-MCAST/TCP)
  if ( (config->select_rtp_protocol == 0x4) || (config->select_rtp_protocol==0x7) )
  {
    g_object_set (G_OBJECT (bin->src_elem), "protocols", config->select_rtp_protocol, NULL);
    GST_DEBUG_OBJECT(bin->src_elem,
        "RTP Protocol=0x%x (0x4=TCP and 0x7=UDP,TCP,UDPMCAST)----\n",
        config->select_rtp_protocol);
  }
  g_signal_connect (G_OBJECT (bin->src_elem), "pad-added",
                    G_CALLBACK (cb_newpad3), bin);

  g_snprintf (elem_name, sizeof (elem_name), "depay_elem%d", bin->bin_id);
  bin->depay = gst_element_factory_make ("rtph264depay", elem_name);
  if (!bin->depay) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "dec_que%d", bin->bin_id);
  bin->dec_que = gst_element_factory_make ("queue", elem_name);
  if (!bin->dec_que) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "decodebin_elem%d", bin->bin_id);
  bin->decodebin = gst_element_factory_make ("decodebin", elem_name);
  if (!bin->decodebin) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_signal_connect (G_OBJECT (bin->decodebin), "pad-added",
                    G_CALLBACK (cb_newpad2), bin);
  g_signal_connect (G_OBJECT (bin->decodebin), "child-added",
                    G_CALLBACK (decodebin_child_added), config);


  g_snprintf (elem_name, sizeof (elem_name), "src_que%d", bin->bin_id);
  bin->cap_filter =
      gst_element_factory_make (NVDS_ELEM_QUEUE, elem_name);
  if (!bin->cap_filter) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_mutex_init(&bin->bin_lock);
  if (config->dewarper_config.enable)
  {
    if (!create_dewarper_bin (&config->dewarper_config, &bin->dewarper_bin)) {
      g_print ("Failed to create dewarper bin \n");
      goto done;
    }
    gst_bin_add_many (GST_BIN (bin->bin), bin->src_elem, bin->depay,
        bin->dec_que,
        bin->decodebin, bin->dewarper_bin.bin, bin->cap_filter, NULL);
  }
  else
  {
    gst_bin_add_many (GST_BIN (bin->bin), bin->src_elem, bin->depay,
        bin->dec_que,
        bin->decodebin, bin->cap_filter, NULL);
  }


  NVGSTDS_LINK_ELEMENT (bin->depay, bin->dec_que);
  NVGSTDS_LINK_ELEMENT (bin->dec_que, bin->decodebin);

  if (config->dewarper_config.enable)
  {
    NVGSTDS_LINK_ELEMENT (bin->cap_filter, bin->dewarper_bin.bin);
    NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->dewarper_bin.bin, "src");
  }
  else
    NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->cap_filter, "src");

  ret = TRUE;

  GST_CAT_DEBUG (NVDS_APP,
                 "Decode bin created. Waiting for a new pad from decodebin to link");

  done:

  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

static gboolean
create_uridecode_src_bin (NvDsSourceConfig * config, NvDsSrcBin * bin)
{
  gboolean ret = FALSE;

  bin->src_elem = gst_element_factory_make (NVDS_ELEM_SRC_URI, "src_elem");
  if (!bin->src_elem) {
    NVGSTDS_ERR_MSG_V ("Could not create element 'src_elem'");
    goto done;
  }

  if (config->dewarper_config.enable)
  {
    if (!create_dewarper_bin (&config->dewarper_config, &bin->dewarper_bin)) {
      g_print ("Creating Dewarper bin failed \n");
      goto done;
    }
  }
  bin->latency = config->latency;

  if (g_strrstr(config->uri,"file:/")){
    config->live_source = FALSE;
  }

  g_object_set (G_OBJECT (bin->src_elem), "uri", config->uri, NULL);
  g_signal_connect (G_OBJECT (bin->src_elem), "pad-added",
      G_CALLBACK (cb_newpad), bin);
  g_signal_connect (G_OBJECT (bin->src_elem), "child-added",
                    G_CALLBACK (decodebin_child_added), config);
   g_signal_connect (G_OBJECT (bin->src_elem), "source-setup",
       G_CALLBACK (cb_sourcesetup), bin);
  bin->cap_filter =
      gst_element_factory_make (NVDS_ELEM_QUEUE, "queue");
  if (!bin->cap_filter) {
    NVGSTDS_ERR_MSG_V ("Could not create 'queue'");
    goto done;
  }

  g_object_set_data (G_OBJECT (bin->cap_filter), SRC_CONFIG_KEY, config);

  gst_bin_add_many (GST_BIN (bin->bin), bin->src_elem, bin->cap_filter, NULL);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->cap_filter, "src");

  bin->fakesink =
      gst_element_factory_make ("fakesink", "src_fakesink");
  if (!bin->fakesink) {
    NVGSTDS_ERR_MSG_V ("Could not create 'src_fakesink'");
    goto done;
  }

  bin->fakesink_queue =
      gst_element_factory_make ("queue", "fakequeue");
  if (!bin->fakesink_queue) {
    NVGSTDS_ERR_MSG_V ("Could not create 'fakequeue'");
    goto done;
  }

  bin->tee =
      gst_element_factory_make ("tee", NULL);
  if (!bin->tee) {
    NVGSTDS_ERR_MSG_V ("Could not create 'tee'");
    goto done;
  }
  gst_bin_add_many (GST_BIN (bin->bin), bin->fakesink, bin->tee, bin->fakesink_queue, NULL);


  NVGSTDS_LINK_ELEMENT (bin->fakesink_queue, bin->fakesink);

  if (config->dewarper_config.enable)
  {
    gst_bin_add_many (GST_BIN (bin->bin), bin->dewarper_bin.bin, NULL);
    NVGSTDS_LINK_ELEMENT (bin->tee,  bin->dewarper_bin.bin);
    NVGSTDS_LINK_ELEMENT (bin->dewarper_bin.bin, bin->cap_filter);
  }
  else
  {
    link_element_to_tee_src_pad (bin->tee, bin->cap_filter);
  }
  link_element_to_tee_src_pad (bin->tee, bin->fakesink_queue);

  g_object_set (G_OBJECT (bin->fakesink), "sync", FALSE, "async", FALSE, NULL);

  ret = TRUE;

  GST_CAT_DEBUG (NVDS_APP,
      "Decode bin created. Waiting for a new pad from decodebin to link");

done:

  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

gboolean
create_source_bin (NvDsSourceConfig *config, NvDsSrcBin *bin)
{
  bin->bin = gst_bin_new ("src_bin");
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create 'src_bin'");
    return FALSE;
  }

  switch (config->type) {
    case NV_DS_SOURCE_CAMERA_V4L2:
      if (!create_camera_source_bin (config, bin)) {
        return FALSE;
      }
      break;
    case NV_DS_SOURCE_URI:
      if (!create_uridecode_src_bin (config, bin)) {
        return FALSE;
      }
      bin->live_source = config->live_source;
      break;
    case NV_DS_SOURCE_RTSP:
      if (!create_rtsp_src_bin (config, bin)) {
        return FALSE;
      }
      break;
    default:
      NVGSTDS_ERR_MSG_V ("Source type not yet implemented!\n");
      return FALSE;
  }

  GST_CAT_DEBUG (NVDS_APP, "Source bin created");

  return TRUE;
}

gboolean
create_multi_source_bin (guint num_sub_bins, NvDsSourceConfig *configs,
                         NvDsSrcParentBin *bin)
{
  gboolean ret = FALSE;
  guint i = 0;

  bin->reset_thread = NULL;

  bin->bin = gst_bin_new ("multi_src_bin");
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create element 'multi_src_bin'");
    goto done;
  }

  g_object_set (bin->bin, "message-forward", TRUE, NULL);

  bin->streammux = gst_element_factory_make (NVDS_ELEM_STREAM_MUX, "src_bin_muxer");
  if (!bin->streammux) {
    NVGSTDS_ERR_MSG_V ("Failed to create element 'src_bin_muxer'");
    goto done;
  }
  gst_bin_add (GST_BIN (bin->bin), bin->streammux);

  for (i = 0; i < num_sub_bins; i++) {
    if (!configs[i].enable) {
      continue;
    }

    gchar elem_name[50];
    g_snprintf (elem_name, sizeof (elem_name), "src_sub_bin%d", i);
    bin->sub_bins[i].bin = gst_bin_new (elem_name);
    if (!bin->sub_bins[i].bin) {
      NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
      goto done;
    }

    bin->sub_bins[i].bin_id = i;
    configs[i].live_source = TRUE;
    bin->live_source = TRUE;
    bin->sub_bins[i].eos_done = TRUE;
    bin->sub_bins[i].reset_done = TRUE;

    switch (configs[i].type) {
      case NV_DS_SOURCE_CAMERA_V4L2:
        if (!create_camera_source_bin (&configs[i], &bin->sub_bins[i])) {
          return FALSE;
        }
        break;
      case NV_DS_SOURCE_URI:
        if (!create_uridecode_src_bin (&configs[i], &bin->sub_bins[i])) {
          return FALSE;
        }
        bin->live_source = configs[i].live_source;
        break;
      case NV_DS_SOURCE_RTSP:
        if (!create_rtsp_src_bin (&configs[i], &bin->sub_bins[i])) {
          return FALSE;
        }
        break;
      default:
        NVGSTDS_ERR_MSG_V ("Source type not yet implemented!\n");
        return FALSE;
    }

    gst_bin_add (GST_BIN (bin->bin), bin->sub_bins[i].bin);

    if (!link_element_to_streammux_sink_pad (bin->streammux,
                                             bin->sub_bins[i].bin, i)) {
      goto done;
    }
    bin->num_bins++;
  }

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->streammux, "src");

  ret = TRUE;

done:
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

gboolean
reset_source_pipeline (gpointer data)
{
  NvDsSrcBin *src_bin = (NvDsSrcBin *) data;

  if (gst_element_set_state (src_bin->bin, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    GST_ERROR_OBJECT (src_bin->bin, "Can't set source bin to NULL");
    return FALSE;
  }
  g_print ("Reset source pipeline %s %p\n,", __func__, src_bin);

  GST_CAT_INFO (NVDS_APP,"Reset source pipeline %s %p\n,",__func__,src_bin);
  if (!gst_element_sync_state_with_parent (src_bin->bin)) {
    GST_ERROR_OBJECT (src_bin->bin, "Couldn't sync state with parent");
  }
  return FALSE;
}

gboolean
set_source_to_playing (gpointer data)
{
  NvDsSrcBin *subBin = (NvDsSrcBin *) data;
  if (subBin->reconfiguring) {
    gst_element_set_state (subBin->bin, GST_STATE_PLAYING);
    GST_CAT_INFO (NVDS_APP,"Reconfiguring %s  %p\n,",__func__,subBin);

    subBin->reconfiguring = FALSE;
  }
  return FALSE;
}

gpointer
reset_encodebin (gpointer data)
{
  NvDsSrcBin *src_bin = (NvDsSrcBin *) data;
  g_usleep (10000);
  GST_CAT_INFO (NVDS_APP,"Reset called %s %p\n,",__func__,src_bin);

  GST_CAT_INFO (NVDS_APP,"Reset setting null for sink %s %p\n,",__func__,src_bin);
  //g_mutex_lock(&src_bin->bin_lock);
  src_bin->reset_done = TRUE;
  //g_mutex_unlock(&src_bin->bin_lock);

  return NULL;
}
