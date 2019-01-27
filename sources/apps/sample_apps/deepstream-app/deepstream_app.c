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

#include <gst/gst.h>
#include <string.h>
#include <math.h>

#include "deepstream_app.h"
#include "gstnvstreammeta.h"

GST_DEBUG_CATEGORY_EXTERN (NVDS_APP);

GQuark _dsmeta_quark;

#define CEIL(a,b) ((a + b - 1) / b)

/**
 * callback function to receive messages from components
 * in the pipeline.
 */
static gboolean
bus_callback (GstBus *bus, GstMessage *message, gpointer data)
{
  AppCtx *appCtx = (AppCtx *) data;
  GST_CAT_DEBUG (NVDS_APP,
      "Received message on bus: source %s, msg_type %s",
      GST_MESSAGE_SRC_NAME (message), GST_MESSAGE_TYPE_NAME (message));
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_INFO:{
      GError *error = NULL;
      gchar *debuginfo = NULL;
      gst_message_parse_info (message, &error, &debuginfo);
      g_printerr ("INFO from %s: %s\n",
          GST_OBJECT_NAME (message->src), error->message);
      if (debuginfo) {
        g_printerr ("Debug info: %s\n", debuginfo);
      }
      g_error_free (error);
      g_free (debuginfo);
      break;
    }
    case GST_MESSAGE_WARNING:{
      GError *error = NULL;
      gchar *debuginfo = NULL;
      gst_message_parse_warning (message, &error, &debuginfo);
      g_printerr ("WARNING from %s: %s\n",
          GST_OBJECT_NAME (message->src), error->message);
      if (debuginfo) {
        g_printerr ("Debug info: %s\n", debuginfo);
      }
      g_error_free (error);
      g_free (debuginfo);
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *error = NULL;
      gchar *debuginfo = NULL;
      gst_message_parse_error (message, &error, &debuginfo);
      g_printerr ("ERROR from %s: %s\n",
          GST_OBJECT_NAME (message->src), error->message);
      if (debuginfo) {
        g_printerr ("Debug info: %s\n", debuginfo);
      }
      g_error_free (error);
      g_free (debuginfo);
      appCtx->return_value = -1;
      appCtx->quit = TRUE;
      break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
      GstState oldstate, newstate;
      gst_message_parse_state_changed (message, &oldstate, &newstate, NULL);
      if (GST_ELEMENT (GST_MESSAGE_SRC (message)) == appCtx->pipeline.pipeline) {
        switch (newstate) {
          case GST_STATE_PLAYING:
            NVGSTDS_INFO_MSG_V ("Pipeline running\n");
            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (appCtx->pipeline.
                    pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "ds-app-playing");
            break;
          case GST_STATE_PAUSED:
            if (oldstate == GST_STATE_PLAYING) {
              NVGSTDS_INFO_MSG_V ("Pipeline paused\n");
            }
            break;
          case GST_STATE_READY:
            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (appCtx->
                    pipeline.pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                "ds-app-ready");
            if (oldstate == GST_STATE_NULL) {
              NVGSTDS_INFO_MSG_V ("Pipeline ready\n");
            } else {
              NVGSTDS_INFO_MSG_V ("Pipeline stopped\n");
            }
            break;
          case GST_STATE_NULL:
            g_mutex_lock (&appCtx->app_lock);
            g_cond_broadcast (&appCtx->app_cond);
            g_mutex_unlock (&appCtx->app_lock);
            break;
          default:
            break;
        }
      }
      break;
    }
    case GST_MESSAGE_EOS: {
      /*
       * In normal scenario, this would use g_main_loop_quit() to exit the
       * loop and release the resources. Since this application might be
       * running multiple pipelines through configuration files, it should wait
       * till all pipelines are done.
       */
      NVGSTDS_INFO_MSG_V ("Received EOS. Exiting ...\n");
      appCtx->quit = TRUE;
      return FALSE;
      break;
    }
    default:
      break;
  }
  return TRUE;
}

/**
 * Function to dump bounding box data in kitti format. For this to work,
 * property "gie-kitti-output-dir" must be set in configuration file.
 * Data of different sources and frames is dumped in separate file.
 */
static gboolean
for_each_meta_get_kitti (GstBuffer *buffer, gpointer user_data)
{
  AppCtx *appCtx = (AppCtx *) user_data;
  guint j;
  gchar bbox_file[1024] = { 0 };
  GstNvStreamMeta *streamMeta = NULL;
  GstMeta *gst_meta = NULL;
  NvDsMeta *meta = NULL;
  gpointer state = NULL;
  streamMeta = gst_buffer_get_nvstream_meta (buffer);
  NvDsGieConfig *gie_config = &appCtx->config.primary_gie_config;
  FILE *bbox_params_dump_file = NULL;

  if (streamMeta != NULL) {
    while (1) {
      /* Iterate for each NVDS_META_FRAME_INFO*/
      gst_meta = gst_buffer_iterate_meta(buffer, &state);

      if (gst_meta == NULL)
        break;

      meta = (NvDsMeta *)gst_meta;
      /* will be set for each frame*/
      if (meta->meta_type == NVDS_META_FRAME_INFO) {
        NvDsFrameMeta * bbox_params = (NvDsFrameMeta *) (meta->meta_data);
        guint batch_id = bbox_params->batch_id;
        guint stream_id = streamMeta->stream_id[batch_id];
        gfloat scalex = 1.0, scaley = 1.0;
        if (appCtx->config.streammux_config.pipeline_width &&
            appCtx->config.streammux_config.pipeline_height) {
          scalex = (gfloat)streamMeta->original_width[batch_id] /
              appCtx->config.streammux_config.pipeline_width;
          scaley = (gfloat)streamMeta->original_height[batch_id]/
              appCtx->config.streammux_config.pipeline_height;
        }
        if (appCtx->config.bbox_dir_path) {
          g_snprintf (bbox_file, sizeof (bbox_file) - 1,
                      "%s/%02u_%03u_%06lu.txt", appCtx->config.bbox_dir_path,
                      appCtx->index, stream_id,
                      streamMeta->stream_frame_num[batch_id]);
          bbox_params_dump_file = fopen (bbox_file, "w");
        }
        for (j = 0; j < bbox_params->num_rects; j++) {
          // Dump bboxes in KITTI format
          if (bbox_params_dump_file) {
            int left =
                (int)(bbox_params->obj_params[j].rect_params.left * scalex);
            int top =
                (int)(bbox_params->obj_params[j].rect_params.top * scaley);
            int right =
                left + (int)(bbox_params->obj_params[j].rect_params.width * scalex);
            int bottom =
                top + (int)(bbox_params->obj_params[j].rect_params.height * scaley);
            int class_index = bbox_params->obj_params[j].class_id;
            char *text = gie_config->labels[class_index][0];
            fprintf (bbox_params_dump_file,
                     "%s 0.0 0 0.0 %d.00 %d.00 %d.00 %d.00 0.0 0.0 0.0 0.0 0.0 0.0 0.0\n",
                     text, left, top, right, bottom);
          }
        }

        if (bbox_params_dump_file) {
          fclose (bbox_params_dump_file);
          bbox_params_dump_file = NULL;
        }
      }
    }
  }
  return TRUE;
}

/**
 * Function to process the attached metadata. This is just for demonstration
 * and can be removed if not required.
 * Here it demonstrates to use bounding boxes of different color and size for
 * different type / class of objects.
 * It also demonstrates how to join the different labels(PGIE + SGIEs)
 * of an object to form a single string.
 */
static gboolean
for_each_meta (GstBuffer *buffer, GstMeta **meta, gpointer user_data)
{
  NvDsInstanceBin *bin = (NvDsInstanceBin *) user_data;
  AppCtx *appCtx = bin->appCtx;
  guint index = bin->index;
  NvDsInstanceData *data = &appCtx->instance_data[index];
  NvDsFrameMeta **params2 = (NvDsFrameMeta **) data->params_list;
  NvDsFrameMeta *params;
  guint j, k;
  GstMeta *meta1 = *meta;
  gchar *str_ins_pos;

  // For single source always display text either with demuxer or with tiler
  if (!appCtx->config.tiled_display_config.enable ||
      appCtx->config.num_source_sub_bins == 1) {
    appCtx->show_bbox_text = 1;
  }

  if (!gst_meta_api_type_has_tag (meta1->info->api, _dsmeta_quark)) {
    return TRUE;
  }

  if (((NvDsMeta *) meta1)->meta_type != NVDS_META_FRAME_INFO) {
    return TRUE;
  }

  params = ((NvDsMeta *) meta1)->meta_data;
  if (!params) {
    return TRUE;
  }

  for (j = 0; j < params->num_rects; j++) {
    NvDsGieConfig *gie_config = NULL;
    gint class_index = params->obj_params[j].class_id;
    gint i;

    gie_config = &appCtx->config.primary_gie_config;

    if (params->gie_type == 1) {
      gie_config = &appCtx->config.primary_gie_config;
    } else if (params->gie_type == 2) {
      for (i = 0; i < (gint) appCtx->config.num_secondary_gie_sub_bins; i++) {
        gie_config = &appCtx->config.secondary_gie_sub_bin_config[i];
        if (gie_config->unique_id == (guint) params->gie_unique_id)
          break;
        gie_config = NULL;
      }
    }

    if (params->obj_params[j].text_params.display_text) {
      params->num_strings--;
      g_free (params->obj_params[j].text_params.display_text);
      params->obj_params[j].text_params.display_text = NULL;
    }
    if (appCtx->show_bbox_text) {
      params->obj_params[j].text_params.display_text = g_malloc (128);
      params->obj_params[j].text_params.display_text[0] = '\0';
      str_ins_pos = params->obj_params[j].text_params.display_text;
      params->num_strings++;
    }

    if (params->gie_type == 1 || params->gie_type == 2) {
      if (g_hash_table_contains (gie_config->bbox_border_color_table,
              class_index + (gchar *) NULL)) {
        params->obj_params[j].rect_params.border_color =
            *((NvOSD_ColorParams *) g_hash_table_lookup (gie_config->
                bbox_border_color_table, class_index + (gchar *) NULL));
      } else {
        params->obj_params[j].rect_params.border_color =
            gie_config->bbox_border_color;
      }
      params->obj_params[j].rect_params.border_width =
          appCtx->config.osd_config.border_width;

      if (g_hash_table_contains (gie_config->bbox_bg_color_table,
              class_index + (gchar *) NULL)) {
        params->obj_params[j].rect_params.has_bg_color = 1;
        params->obj_params[j].rect_params.bg_color =
            *((NvOSD_ColorParams *) g_hash_table_lookup (gie_config->
                bbox_bg_color_table, class_index + (gchar *) NULL));
      } else {
        params->obj_params[j].rect_params.has_bg_color = 0;
      }
    }

    if (appCtx->show_bbox_text) {
      // Only for PRI GIE ID
      if (params->gie_type == 1) {
        if (class_index >= 0) {
          if (class_index >= (gint) gie_config->n_labels) {
            g_snprintf (str_ins_pos, MAX_BBOX_TEXT_SIZE - 1, "Unknown(%d)",
                params->obj_params[j].class_id);
          } else {
            g_snprintf (str_ins_pos, MAX_BBOX_TEXT_SIZE - 1, "%s",
                gie_config->labels[class_index][0]);
          }
        } else {
          for (int l = 0; l < params->obj_params[j].attr_info[gie_config->unique_id].num_attrs; l++) {
            gint label_id = params->obj_params[j].attr_info[gie_config->unique_id].attrs[l].attr_id;
            guint label_output_id =
              params->obj_params[j].attr_info[gie_config->unique_id].attrs[l].attr_val;

            if (label_output_id < 0 || label_id >= gie_config->n_labels ||
                label_output_id >= gie_config->n_label_outputs[label_id]) {
              continue;
            }
            sprintf (str_ins_pos, "%s",
                gie_config->labels[label_id][label_output_id]);
            str_ins_pos += strlen (str_ins_pos);
          }
        }

        str_ins_pos += strlen (str_ins_pos);

        if (params->obj_params[j].tracking_id != -1) {
          sprintf (str_ins_pos, " %d", params->obj_params[j].tracking_id);
          str_ins_pos += strlen (str_ins_pos);
        }
      }

      for (k = 0; k < NVDS_MAX_ATTRIBUTES; k++) {
        NvDsGieConfig *gieconfig = NULL;
        gint l;

        if (k == appCtx->config.primary_gie_config.unique_id) {
          continue;
        }

        if (params->obj_params[j].attr_info[k].is_attr_label) {
          sprintf (str_ins_pos, " %s",
                   params->obj_params[j].attr_info[k].attr_label);
          str_ins_pos += strlen (str_ins_pos);
          continue;
        }

        for (l = 0; l < MAX_SECONDARY_GIE_BINS; l++) {
          if (appCtx->config.secondary_gie_sub_bin_config[l].unique_id == k) {
            gieconfig = &appCtx->config.secondary_gie_sub_bin_config[l];
            break;
          }
        }

        if (!gieconfig) {
          if (params->obj_params[j].attr_info[k].is_attr_label) {
            sprintf (str_ins_pos, " %s",
                     params->obj_params[j].attr_info[k].attr_label);
            str_ins_pos += strlen (str_ins_pos);
          }
          continue;
        }

        if (params->obj_params[j].attr_info[k].num_attrs) {
          sprintf (str_ins_pos, " ");
          str_ins_pos += strlen (str_ins_pos);
        }

        for (l = 0; l < params->obj_params[j].attr_info[k].num_attrs; l++) {
          gint label_id = params->obj_params[j].attr_info[k].attrs[l].attr_id;
          guint label_output_id =
              params->obj_params[j].attr_info[k].attrs[l].attr_val;

          if (label_output_id < 0
              || label_output_id >= gieconfig->n_label_outputs[label_id]) {
            continue;
          }
          sprintf (str_ins_pos, "%s ",
                   gieconfig->labels[label_id][label_output_id]);
          str_ins_pos += strlen (str_ins_pos);
        }
      }

      params->obj_params[j].text_params.x_offset =
          params->obj_params[j].rect_params.left;
      params->obj_params[j].text_params.y_offset =
          params->obj_params[j].rect_params.top - 30;
      params->obj_params[j].text_params.font_params.font_color =
          appCtx->config.osd_config.text_color;
      params->obj_params[j].text_params.font_params.font_size =
          appCtx->config.osd_config.text_size;
      params->obj_params[j].text_params.font_params.font_name =
          appCtx->config.osd_config.font;
      if (appCtx->config.osd_config.text_has_bg) {
        params->obj_params[j].text_params.set_bg_clr = 1;
        params->obj_params[j].text_params.text_bg_clr =
            appCtx->config.osd_config.text_bg_color;
      }
    }
  }

  params2[data->bbox_list_size++] = params;

  return TRUE;
}

/**
 * Function which processes the inferred buffer and its metadata.
 * It also gives opportunity to attach application specific
 * metadata (e.g. clock, analytics output etc.).
 */
static void
process_buffer (GstBuffer *buf, AppCtx *appCtx, guint index)
{
  NvDsInstanceData *data = &appCtx->instance_data[index];
  NvDsFrameMeta *app_overlay_params;
  NvDsMeta *meta;
  guint i;

  data->frame_num++;

  memset (data->params_list, 0, sizeof (data->params_list));
  data->bbox_list_size = 0;
  // call "for_each_meta" to process metadata.
  gst_buffer_foreach_meta (buf, for_each_meta,
                           &appCtx->pipeline.instance_bins[index]);

  /* Opportunity to modify the processed metadata or do analytics based on
   * type of object e.g. maintaining count of particular type of car.
   */
  if (appCtx->all_bbox_generated_cb) {
    appCtx->all_bbox_generated_cb (appCtx, buf, data->params_list, index);
  }
  data->bbox_list_size = 0;

  /*
   * callback to attach application specific additional metadata.
   */
  if (appCtx->overlay_graphics_cb) {
    app_overlay_params = nvds_meta_pool_get_free_meta (&appCtx->meta_pool);
    if (appCtx->overlay_graphics_cb (appCtx, buf, app_overlay_params, index) &&
        (app_overlay_params->num_rects > 0
            || app_overlay_params->num_strings > 0)) {
      data->params_list[data->bbox_list_size] = app_overlay_params;
      data->bbox_list_size++;
    } else {
      nvds_meta_pool_release_meta_to_pool (app_overlay_params);
    }
  }

  for (i = 0; i < data->bbox_list_size; i++) {
    NvDsFrameMeta *params1 = data->params_list[i];
    params1->nvosd_mode = NV_OSD_MODE_GPU;

    if (params1->num_rects > 0 || params1->num_strings > 0) {
      meta = gst_buffer_add_nvds_meta (buf, params1,
          nvds_meta_pool_release_meta_to_pool);
      if (meta) {
        meta->meta_type = NVDS_META_FRAME_INFO;
      } else {
        nvds_meta_pool_release_meta_to_pool (params1);
      }
    } else {
      nvds_meta_pool_release_meta_to_pool (params1);
    }
  }
}

/**
 * Buffer probe function to get the results of primary infer.
 * Here it demonstrates the use by dumping bounding box coordinates in
 * kitti format.
 */
static GstPadProbeReturn
gie_primary_processing_done_buf_prob (GstPad *pad, GstPadProbeInfo *info,
    gpointer u_data)
{
  GstBuffer *buf = (GstBuffer *) info->data;
  AppCtx *appCtx = (AppCtx *) u_data;

  for_each_meta_get_kitti (buf, appCtx);

  return GST_PAD_PROBE_OK;
}

/**
 * Probe function to get results after all inferences(Primary + Secondary)
 * are done. This will be just before OSD or sink (in case OSD is disabled).
 */
static GstPadProbeReturn
gie_processing_done_buf_prob (GstPad *pad, GstPadProbeInfo *info,
    gpointer u_data)
{
  GstBuffer *buf = (GstBuffer *) info->data;
  NvDsInstanceBin *bin = (NvDsInstanceBin *) u_data;
  guint index = bin->index;
  AppCtx *appCtx = bin->appCtx;

  if (gst_buffer_is_writable (buf))
    process_buffer (buf, appCtx, index);
  return GST_PAD_PROBE_OK;
}

/**
 * Buffer probe function after tracker.
 */
static GstPadProbeReturn
tracking_done_buf_prob (GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
  NvDsInstanceBin *bin = (NvDsInstanceBin *) u_data;
  guint index = bin->index;
  AppCtx *appCtx = bin->appCtx;
  GstBuffer *buf = (GstBuffer *) info->data;
  GstMeta *meta;
  gpointer state = NULL;

  while ((meta = gst_buffer_iterate_meta (buf, &state))) {
    NvDsFrameMeta *params[2] = { NULL };
    if (!gst_meta_api_type_has_tag (meta->info->api, _dsmeta_quark)) {
      continue;
    }

    params[0] = (NvDsFrameMeta *) ((NvDsMeta *) meta)->meta_data;
    if (params[0]->gie_type == 1) {
      appCtx->primary_bbox_generated_cb (appCtx, buf, params, index);
    }
  }
  return GST_PAD_PROBE_OK;
}

/*
 * Function to seek the source stream to start.
 * It is required to play the stream in loop.
 */
static gboolean
seek_decode (gpointer data)
{
  NvDsSrcBin *bin = (NvDsSrcBin *) data;
  gboolean ret = TRUE;

  gst_element_set_state (bin->bin, GST_STATE_PAUSED);

  gst_pad_send_event (gst_element_get_static_pad (bin->tee, "sink"),
                      gst_event_new_flush_start ());
  gst_pad_send_event (gst_element_get_static_pad (bin->tee, "sink"),
                      gst_event_new_flush_stop (TRUE));

  ret = gst_element_seek (bin->bin, 1.0, GST_FORMAT_TIME,
      (GstSeekFlags) (GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_FLUSH),
      GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  if (!ret)
    GST_WARNING ("Error in seeking pipeline");

  gst_element_set_state (bin->bin, GST_STATE_PLAYING);

  return FALSE;
}

/**
 * Probe function to drop certain events to support custom
 * logic of looping of each source stream.
 */
static GstPadProbeReturn
tiler_restart_stream_buf_prob (GstPad *pad, GstPadProbeInfo *info,
                               gpointer u_data)
{
  GstEvent *event = GST_EVENT (info->data);
  NvDsSrcBin *bin = (NvDsSrcBin *) u_data;
  if ((info->type & GST_PAD_PROBE_TYPE_EVENT_BOTH)) {
    if (GST_EVENT_TYPE (event) == GST_EVENT_EOS) {
      g_timeout_add (1, seek_decode, bin);
    }

    if (GST_EVENT_TYPE (event) == GST_EVENT_SEGMENT) {
      GstSegment *segment;

      gst_event_parse_segment (event, (const GstSegment **) &segment);
      segment->base = bin->accumulated_base;
      bin->accumulated_base += segment->stop;
    }
    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_EOS:
      /* QOS events from downstream sink elements cause decoder to drop
       * frames after looping the file since the timestamps reset to 0.
       * We should drop the QOS events since we have custom logic for
       * looping individual sources. */
      case GST_EVENT_QOS:
      case GST_EVENT_FLUSH_START:
      case GST_EVENT_FLUSH_STOP:
      case GST_EVENT_SEEK:
        return GST_PAD_PROBE_DROP;
      default:
        break;
    }
  }
  return GST_PAD_PROBE_OK;
}

/**
 * Function to add components to pipeline which are dependent on number
 * of streams. These components work on single buffer. If tiling is being
 * used then single instance will be created otherwise < N > such instances
 * will be created for < N > streams
 */
static gboolean
create_processing_instance (AppCtx *appCtx, guint index)
{
  gboolean ret = FALSE;
  NvDsConfig *config = &appCtx->config;
  NvDsInstanceBin *instance_bin = &appCtx->pipeline.instance_bins[index];
  GstElement *last_elem;
  gchar elem_name[32];

  instance_bin->index = index;
  instance_bin->appCtx = appCtx;

  g_snprintf (elem_name, 32, "processing_bin_%d", index);
  instance_bin->bin = gst_bin_new (elem_name);

  if (!create_sink_bin (config->num_sink_sub_bins,
          config->sink_bin_sub_bin_config, &instance_bin->sink_bin, index)) {
    goto done;
  }

  gst_bin_add (GST_BIN (instance_bin->bin), instance_bin->sink_bin.bin);
  last_elem = instance_bin->sink_bin.bin;

  if (config->osd_config.enable) {
    if (!create_osd_bin (&config->osd_config, &instance_bin->osd_bin)) {
      goto done;
    }

    gst_bin_add (GST_BIN (instance_bin->bin), instance_bin->osd_bin.bin);

    NVGSTDS_LINK_ELEMENT (instance_bin->osd_bin.bin, last_elem);

    last_elem = instance_bin->osd_bin.bin;
  }

  NVGSTDS_BIN_ADD_GHOST_PAD (instance_bin->bin, last_elem, "sink");
  if (config->osd_config.enable) {
    NVGSTDS_ELEM_ADD_PROBE (instance_bin->all_bbox_buffer_probe_id,
        instance_bin->osd_bin.nvosd, "sink",
        gie_processing_done_buf_prob, GST_PAD_PROBE_TYPE_BUFFER, instance_bin);
  } else {
    NVGSTDS_ELEM_ADD_PROBE (instance_bin->all_bbox_buffer_probe_id,
        instance_bin->sink_bin.bin, "sink",
        gie_processing_done_buf_prob, GST_PAD_PROBE_TYPE_BUFFER, instance_bin);
  }

  ret = TRUE;
done:
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

/**
 * Function to create common elements(Primary infer, tracker, secondary infer)
 * of the pipeline. These components operate on muxed data from all the
 * streams. So they are independent of number of streams in the pipeline.
 */
gboolean
create_common_elements (NvDsConfig *config, NvDsPipeline *pipeline,
    GstElement **sink_elem, GstElement **src_elem,
    bbox_generated_callback primary_bbox_generated_cb)
{
  gboolean ret = FALSE;
  *sink_elem = *src_elem = NULL;
  if (config->primary_gie_config.enable) {
    if (config->num_secondary_gie_sub_bins > 0) {
      if (!create_secondary_gie_bin (config->num_secondary_gie_sub_bins,
                                     config->primary_gie_config.unique_id,
                                     config->secondary_gie_sub_bin_config,
                                     &pipeline->common_elements.secondary_gie_bin)) {
        goto done;
      }
      gst_bin_add (GST_BIN (pipeline->pipeline),
                   pipeline->common_elements.secondary_gie_bin.bin);
      if (!*src_elem) {
        *src_elem = pipeline->common_elements.secondary_gie_bin.bin;
      }
      if (*sink_elem) {
        NVGSTDS_LINK_ELEMENT (pipeline->common_elements.secondary_gie_bin.bin,
                              *sink_elem);
      }
      *sink_elem = pipeline->common_elements.secondary_gie_bin.bin;
    }
  }

  if (config->tracker_config.enable) {
    if (!create_tracking_bin (&config->tracker_config,
                              &pipeline->common_elements.tracker_bin)) {
      g_print("creating tracker bin failed\n");
      goto done;
    }
    gst_bin_add (GST_BIN (pipeline->pipeline),
                 pipeline->common_elements.tracker_bin.bin);
    if (!*src_elem) {
      *src_elem = pipeline->common_elements.tracker_bin.bin;
    }
    if (*sink_elem) {
      NVGSTDS_LINK_ELEMENT (pipeline->common_elements.tracker_bin.bin, *sink_elem);
    }
    *sink_elem = pipeline->common_elements.tracker_bin.bin;
  }

  if (config->primary_gie_config.enable) {
    if (!create_primary_gie_bin (&config->primary_gie_config,
                                 &pipeline->common_elements.primary_gie_bin)) {
      goto done;
    }
    gst_bin_add (GST_BIN (pipeline->pipeline),
                 pipeline->common_elements.primary_gie_bin.bin);
    if (*sink_elem) {
      NVGSTDS_LINK_ELEMENT (pipeline->common_elements.primary_gie_bin.bin,
                            *sink_elem);
    }
    *sink_elem = pipeline->common_elements.primary_gie_bin.bin;
    if (!*src_elem) {
      *src_elem = pipeline->common_elements.primary_gie_bin.bin;
    }
    NVGSTDS_ELEM_ADD_PROBE (pipeline->common_elements.primary_bbox_buffer_probe_id,
        pipeline->common_elements.primary_gie_bin.bin, "src",
        gie_primary_processing_done_buf_prob, GST_PAD_PROBE_TYPE_BUFFER,
        pipeline->common_elements.appCtx);
  }

  if (config->primary_gie_config.enable && primary_bbox_generated_cb) {
    if (config->tracker_config.enable) {
      NVGSTDS_ELEM_ADD_PROBE (pipeline->common_elements.primary_bbox_buffer_probe_id,
          pipeline->common_elements.tracker_bin.bin, "src",
          tracking_done_buf_prob, GST_PAD_PROBE_TYPE_BUFFER,
          &pipeline->common_elements);
    } else {
      NVGSTDS_ELEM_ADD_PROBE (pipeline->common_elements.primary_bbox_buffer_probe_id,
          pipeline->common_elements.primary_gie_bin.bin, "src",
          tracking_done_buf_prob, GST_PAD_PROBE_TYPE_BUFFER,
          &pipeline->common_elements);
    }
  }
  ret = TRUE;
done:
  return ret;
}

/**
 * Main function to create the pipeline.
 */
gboolean
create_pipeline (AppCtx *appCtx,
    bbox_generated_callback primary_bbox_generated_cb,
    bbox_generated_callback all_bbox_generated_cb, perf_callback perf_cb,
    overlay_graphics_callback overlay_graphics_cb)
{
  gboolean ret = FALSE;
  NvDsPipeline *pipeline = &appCtx->pipeline;
  NvDsConfig *config = &appCtx->config;
  GstBus *bus;
  GstElement *last_elem;
  GstElement *tmp_elem1;
  GstElement *tmp_elem2;
  guint i;
  GstPad *fps_pad;

  _dsmeta_quark = g_quark_from_static_string (NVDS_META_STRING);

  appCtx->all_bbox_generated_cb = all_bbox_generated_cb;
  appCtx->primary_bbox_generated_cb = primary_bbox_generated_cb;
  appCtx->overlay_graphics_cb = overlay_graphics_cb;

  if (config->osd_config.num_out_buffers < 8) {
    config->osd_config.num_out_buffers = 8;
  }

  pipeline->pipeline = gst_pipeline_new ("pipeline");
  if (!pipeline->pipeline) {
    NVGSTDS_ERR_MSG_V ("Failed to create pipeline");
    goto done;
  }

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline->pipeline));
  pipeline->bus_id = gst_bus_add_watch (bus, bus_callback, appCtx);
  gst_object_unref (bus);

  /*
   * It adds muxer and < N > source components to the pipeline based
   * on the settings in configuration file.
   */
  if (!create_multi_source_bin (config->num_source_sub_bins,
                                config->multi_source_config,
                                &pipeline->multi_src_bin))
    goto done;
  gst_bin_add (GST_BIN (pipeline->pipeline), pipeline->multi_src_bin.bin);


  if (config->streammux_config.is_parsed)
    set_streammux_properties (&config->streammux_config,
        pipeline->multi_src_bin.streammux);

  /*
   * Add probe to drop some events in case looping is required. This is done
   * to support custom logic of looping individual stream instead of whole
   * pipeline
   */
  for (i = 0; i < config->num_source_sub_bins && appCtx->config.file_loop; i++) {
    if (pipeline->multi_src_bin.sub_bins[i].cap_filter)
      NVGSTDS_ELEM_ADD_PROBE (pipeline->multi_src_bin.sub_bins[i].src_buffer_probe,
        pipeline->multi_src_bin.sub_bins[i].cap_filter, "src",
        tiler_restart_stream_buf_prob,
        (GstPadProbeType) (GST_PAD_PROBE_TYPE_EVENT_BOTH |
            GST_PAD_PROBE_TYPE_EVENT_FLUSH),
        &pipeline->multi_src_bin.sub_bins[i]);
  }

  if (config->tiled_display_config.enable) {

    /* Tiler will generate a single composited buffer for all sources. So need
     * to create only one processing instance. */
    if (!create_processing_instance (appCtx, 0)) {
        goto done;
    }

    // create and add tiling component to pipeline.
    if (config->tiled_display_config.columns * config->tiled_display_config.rows <
        config->num_source_sub_bins) {
      if (config->tiled_display_config.columns == 0) {
        config->tiled_display_config.columns =
            (guint) (sqrt (config->num_source_sub_bins) + 0.5);
      }
      config->tiled_display_config.rows =
        (guint) ceil(1.0 * config->num_source_sub_bins / config->tiled_display_config.columns);
      NVGSTDS_WARN_MSG_V("Num of Tiles less than number of sources, readjusting to "
          "%u rows, %u columns", config->tiled_display_config.rows,
          config->tiled_display_config.columns);
    }

    gst_bin_add (GST_BIN (pipeline->pipeline), pipeline->instance_bins[0].bin);
    last_elem = pipeline->instance_bins[0].bin;

    if (!create_tiled_display_bin (&config->tiled_display_config,
                                   &pipeline->tiled_display_bin)) {
      goto done;
    }
    gst_bin_add (GST_BIN (pipeline->pipeline), pipeline->tiled_display_bin.bin);
    NVGSTDS_LINK_ELEMENT (pipeline->tiled_display_bin.bin, last_elem);
    last_elem = pipeline->tiled_display_bin.bin;
  } else {

    /*
     * Create demuxer only if tiled display is disabled.
     */
    pipeline->demuxer =
      gst_element_factory_make (NVDS_ELEM_STREAM_DEMUX, "demuxer");
    if (!pipeline->demuxer) {
      NVGSTDS_ERR_MSG_V("Failed to create element 'demuxer'");
      goto done;
    }
    gst_bin_add (GST_BIN (pipeline->pipeline), pipeline->demuxer);

    for (i = 0; i < config->num_source_sub_bins; i++) {
      gchar pad_name[16];
      gboolean create_instance = FALSE;
      GstPad *demux_src_pad;
      guint j;

      /* Check if any sink has been configured to render/encode output for
       * source index `i`. The processing instance for that source will be
       * created only if atleast one sink has been configured as such.
       */
      for (j = 0; j < config->num_sink_sub_bins; j++) {
        if (config->sink_bin_sub_bin_config[j].enable &&
            config->sink_bin_sub_bin_config[j].source_id == i) {
          create_instance = TRUE;
          break;
        }
      }

      if (!create_instance)
        continue;

      if (!create_processing_instance (appCtx, i)) {
        goto done;
      }
      gst_bin_add (GST_BIN (pipeline->pipeline), pipeline->instance_bins[i].bin);

      g_snprintf (pad_name, 16, "src_%02d", i);
      demux_src_pad = gst_element_get_request_pad (pipeline->demuxer, pad_name);
      NVGSTDS_LINK_ELEMENT_FULL (pipeline->demuxer, pad_name,
          pipeline->instance_bins[i].bin, "sink");
      gst_object_unref (demux_src_pad);
    }

    last_elem = pipeline->demuxer;
  }
  fps_pad = gst_element_get_static_pad (last_elem, "sink");

  pipeline->common_elements.appCtx = appCtx;
  // Decide where in the pipeline the element should be added and add only if
   // enabled
  if (config->dsexample_config.enable) {
    // Create dsexample element bin and set properties
    if (!create_dsexample_bin (&config->dsexample_config,
                               &pipeline->dsexample_bin)) {
      goto done;
    }

    // Add dsexample bin to instance bin
    gst_bin_add (GST_BIN (pipeline->pipeline),
                 pipeline->dsexample_bin.bin);

    // Link this bin to the last element in the bin
    NVGSTDS_LINK_ELEMENT (pipeline->dsexample_bin.bin, last_elem);

    // Set this bin as the last element
    last_elem = pipeline->dsexample_bin.bin;
  }

  // create and add common components to pipeline.
  if (!create_common_elements (config, pipeline, &tmp_elem1, &tmp_elem2,
        primary_bbox_generated_cb)) {
    goto done;
  }

  if (tmp_elem2) {
    NVGSTDS_LINK_ELEMENT (tmp_elem2, last_elem);
    last_elem = tmp_elem1;
  }

  NVGSTDS_LINK_ELEMENT (pipeline->multi_src_bin.bin, last_elem);

  // enable performance measurement and add call back function to receive
  // performance data.
  if (config->enable_perf_measurement) {
    appCtx->perf_struct.context = appCtx;
    enable_perf_measurement (&appCtx->perf_struct, fps_pad,
        pipeline->multi_src_bin.num_bins,
        config->perf_measurement_interval_sec, perf_cb);
  }
  //gst_object_unref (fps_pad);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (appCtx->pipeline.pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "ds-app-null");

  g_mutex_init (&appCtx->app_lock);
  g_cond_init (&appCtx->app_cond);
  ret = TRUE;
done:
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

/**
 * Function to destroy pipeline and release the resources, probes etc.
 */
void
destroy_pipeline (AppCtx *appCtx)
{
  gint64 end_time;
  NvDsConfig *config = &appCtx->config;
  guint i;
  GstBus *bus = NULL;

  end_time = g_get_monotonic_time () + G_TIME_SPAN_SECOND;

  if (!appCtx)
    return;

  if (appCtx->pipeline.demuxer) {
    gst_pad_send_event (gst_element_get_static_pad (appCtx->pipeline.demuxer,
            "sink"), gst_event_new_eos ());
  } else if (appCtx->pipeline.instance_bins[0].sink_bin.bin) {
    gst_pad_send_event (gst_element_get_static_pad (appCtx->pipeline.
            instance_bins[0].sink_bin.bin, "sink"), gst_event_new_eos ());
  }

  g_usleep (100000);

  g_mutex_lock (&appCtx->app_lock);
  if (appCtx->pipeline.pipeline) {
    bus = gst_pipeline_get_bus (GST_PIPELINE (appCtx->pipeline.pipeline));

    while (TRUE) {
      GstMessage *message = gst_bus_pop (bus);
      if (message == NULL)
        break;
      else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
        bus_callback (bus, message, appCtx);
      else
        gst_message_unref (message);
    }
    gst_element_set_state (appCtx->pipeline.pipeline, GST_STATE_NULL);
  }
  g_cond_wait_until (&appCtx->app_cond, &appCtx->app_lock, end_time);
  g_mutex_unlock (&appCtx->app_lock);

  for (i = 0; i < appCtx->config.num_source_sub_bins; i++) {
    NvDsInstanceBin *bin = &appCtx->pipeline.instance_bins[i];
    if (config->osd_config.enable) {
      NVGSTDS_ELEM_REMOVE_PROBE (bin->all_bbox_buffer_probe_id,
          bin->osd_bin.nvosd, "sink");
    } else {
      NVGSTDS_ELEM_REMOVE_PROBE (bin->all_bbox_buffer_probe_id,
          bin->sink_bin.bin, "sink");
    }

    if (config->primary_gie_config.enable) {
      NVGSTDS_ELEM_REMOVE_PROBE (bin->primary_bbox_buffer_probe_id,
                                  bin->primary_gie_bin.bin, "src");
    }

  }
  if (appCtx->pipeline.pipeline) {
      bus = gst_pipeline_get_bus (GST_PIPELINE (appCtx->pipeline.pipeline));
      gst_bus_remove_watch (bus);
      gst_object_unref (bus);
      gst_object_unref (appCtx->pipeline.pipeline);
  }
}

gboolean
pause_pipeline (AppCtx *appCtx)
{
  GstState cur;
  GstState pending;
  GstStateChangeReturn ret;
  GstClockTime timeout = 5 * GST_SECOND / 1000;

  ret =
      gst_element_get_state (appCtx->pipeline.pipeline, &cur, &pending,
      timeout);

  if (ret == GST_STATE_CHANGE_ASYNC) {
    return FALSE;
  }

  if (cur == GST_STATE_PAUSED) {
    return TRUE;
  } else if (cur == GST_STATE_PLAYING) {
    gst_element_set_state (appCtx->pipeline.pipeline, GST_STATE_PAUSED);
    gst_element_get_state (appCtx->pipeline.pipeline, &cur, &pending,
        GST_CLOCK_TIME_NONE);
    pause_perf_measurement (&appCtx->perf_struct);
    return TRUE;
  } else {
    return FALSE;
  }
}

gboolean
resume_pipeline (AppCtx *appCtx)
{
  GstState cur;
  GstState pending;
  GstStateChangeReturn ret;
  GstClockTime timeout = 5 * GST_SECOND / 1000;

  ret =
      gst_element_get_state (appCtx->pipeline.pipeline, &cur, &pending,
      timeout);

  if (ret == GST_STATE_CHANGE_ASYNC) {
    return FALSE;
  }

  if (cur == GST_STATE_PLAYING) {
    return TRUE;
  } else if (cur == GST_STATE_PAUSED) {
    gst_element_set_state (appCtx->pipeline.pipeline, GST_STATE_PLAYING);
    gst_element_get_state (appCtx->pipeline.pipeline, &cur, &pending,
        GST_CLOCK_TIME_NONE);
    resume_perf_measurement (&appCtx->perf_struct);
    return TRUE;
  } else {
    return FALSE;
  }
}

