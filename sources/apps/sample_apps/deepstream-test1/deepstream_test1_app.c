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
#include <glib.h>

#include "gstnvdsmeta.h"

#define MAX_DISPLAY_LEN 64

#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2

gint frame_number = 0;
gchar pgie_classes_str[4][32] = { "Vehicle", "TwoWheeler", "Person",
  "Roadsign"
};

/* osd_sink_pad_buffer_probe  will extract metadata received on OSD sink pad
 * and update params for drawing rectangle, object information etc. */

static GstPadProbeReturn
osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer u_data)
{

  GstMeta *gst_meta = NULL;
  NvDsMeta *nvdsmeta = NULL;
  gpointer state = NULL;
  static GQuark _nvdsmeta_quark = 0;
  GstBuffer *buf = (GstBuffer *) info->data;
  NvDsFrameMeta *frame_meta = NULL;
  guint num_rects = 0, rect_index = 0, l_index = 0;
  NvDsObjectParams *obj_meta = NULL;
  guint i = 0;
  NvOSD_TextParams *txt_params = NULL;
  guint vehicle_count = 0;
  guint person_count = 0;

  if (!_nvdsmeta_quark)
    _nvdsmeta_quark = g_quark_from_static_string (NVDS_META_STRING);

  while ((gst_meta = gst_buffer_iterate_meta (buf, &state))) {
    if (gst_meta_api_type_has_tag (gst_meta->info->api, _nvdsmeta_quark)) {

      nvdsmeta = (NvDsMeta *) gst_meta;

      /* We are interested only in intercepting Meta of type
       * "NVDS_META_FRAME_INFO" as they are from our infer elements. */
      if (nvdsmeta->meta_type == NVDS_META_FRAME_INFO) {
        frame_meta = (NvDsFrameMeta *) nvdsmeta->meta_data;
        if (frame_meta == NULL) {
          g_print ("NvDS Meta contained NULL meta \n");
          return GST_PAD_PROBE_OK;
        }

        /* We reset the num_strings here as we plan to iterate through the
         *  the detected objects and form our own strings.
         *  The pipeline generated strings shall be discarded.
         */
        frame_meta->num_strings = 0;

        num_rects = frame_meta->num_rects;

        /* This means we have num_rects in frame_meta->obj_params,
         * now lets iterate through them */

        for (rect_index = 0; rect_index < num_rects; rect_index++) {
          /* Now using above information we need to form a text that should
           * be displayed on top of the bounding box, so lets form it here. */

          obj_meta = (NvDsObjectParams *) & frame_meta->obj_params[rect_index];

          txt_params = &(obj_meta->text_params);
          if (txt_params->display_text)
            g_free (txt_params->display_text);

          txt_params->display_text = g_malloc0 (MAX_DISPLAY_LEN);

          g_snprintf (txt_params->display_text, MAX_DISPLAY_LEN, "%s ",
              pgie_classes_str[obj_meta->class_id]);

          if (obj_meta->class_id == PGIE_CLASS_ID_VEHICLE)
            vehicle_count++;
          if (obj_meta->class_id == PGIE_CLASS_ID_PERSON)
            person_count++;

          /* Now set the offsets where the string should appear */
          txt_params->x_offset = obj_meta->rect_params.left;
          txt_params->y_offset = obj_meta->rect_params.top - 25;

          /* Font , font-color and font-size */
          txt_params->font_params.font_name = "Arial";
          txt_params->font_params.font_size = 10;
          txt_params->font_params.font_color.red = 1.0;
          txt_params->font_params.font_color.green = 1.0;
          txt_params->font_params.font_color.blue = 1.0;
          txt_params->font_params.font_color.alpha = 1.0;

          /* Text background color */
          txt_params->set_bg_clr = 1;
          txt_params->text_bg_clr.red = 0.0;
          txt_params->text_bg_clr.green = 0.0;
          txt_params->text_bg_clr.blue = 0.0;
          txt_params->text_bg_clr.alpha = 1.0;

          frame_meta->num_strings++;
        }
      }
    }
  }
  g_print ("Frame Number = %d Number of objects = %d "
      "Vehicle Count = %d Person Count = %d\n",
      frame_number, num_rects, vehicle_count, person_count);
  frame_number++;

  return GST_PAD_PROBE_OK;
}

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      if (debug)
        g_printerr ("Error details: %s\n", debug);
      g_free (debug);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop = NULL;
  GstElement *pipeline = NULL, *source = NULL, *h264parser = NULL,
      *decoder = NULL, *sink = NULL, *pgie = NULL, *nvvidconv = NULL,
      *nvosd = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id;
  gulong osd_probe_id = 0;
  GstPad *osd_sink_pad = NULL;

  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <H264 filename>\n", argv[0]);
    return -1;
  }

  /* Standard GStreamer initialization */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create gstreamer elements */
  /* Create Pipeline element that will form a connection of other elements */
  pipeline = gst_pipeline_new ("dstest1-pipeline");

  /* Source element for reading from the file */
  source = gst_element_factory_make ("filesrc", "file-source");

  /* Since the data format in the input file is elementary h264 stream,
   * we need a h264parser */
  h264parser = gst_element_factory_make ("h264parse", "h264-parser");

  /* Use nvdec_h264 for hardware accelerated decode on GPU */
  decoder = gst_element_factory_make ("nvdec_h264", "nvh264-decoder");

  /* Use nvinfer to run inferencing on decoder's output,
   * behaviour of inferencing is set through config file */
  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv = gst_element_factory_make ("nvvidconv", "nvvideo-converter");

  /* Create OSD to draw on the converted RGBA buffer */
  nvosd = gst_element_factory_make ("nvosd", "nv-onscreendisplay");

  /* Finally render the osd output */
  sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");

  if (!pipeline || !source || !h264parser || !decoder || !pgie
      || !nvvidconv || !nvosd || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* we set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", argv[1], NULL);

  /* Set all the necessary properties of the nvinfer element,
   * the necessary ones are : */
  g_object_set (G_OBJECT (pgie),
      "config-file-path", "dstest1_pgie_config.txt", NULL);

  /* we set the osd properties here */
  g_object_set (G_OBJECT (nvosd), "font-size", 15, NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Set up the pipeline */
  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline),
      source, h264parser, decoder, pgie,
      nvvidconv, nvosd, sink, NULL);

  /* we link the elements together */
  /* file-source -> h264-parser -> nvh264-decoder ->
   * nvinfer -> nvvidconv -> nvosd -> video-renderer */
  if (!gst_element_link_many (source, h264parser, decoder, pgie,
      nvvidconv, nvosd, sink, NULL)) {
    g_printerr ("Elements could not be linked. Exiting.\n");
    return -1;
  }

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the osd element, since by that time, the buffer would have
   * had got all the metadata. */
  osd_sink_pad = gst_element_get_static_pad (nvosd, "sink");
  if (!osd_sink_pad)
    g_print ("Unable to get sink pad\n");
  else
    osd_probe_id = gst_pad_add_probe (osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
        osd_sink_pad_buffer_probe, NULL, NULL);

  /* Set the pipeline to "playing" state */
  g_print ("Now playing: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait till pipeline encounters an error or EOS */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  return 0;


}
