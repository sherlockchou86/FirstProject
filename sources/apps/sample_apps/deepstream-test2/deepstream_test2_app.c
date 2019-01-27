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

#define PGIE_CONFIG_FILE  "dstest2_pgie_config.txt"
#define SGIE1_CONFIG_FILE "dstest2_sgie1_config.txt"
#define SGIE2_CONFIG_FILE "dstest2_sgie2_config.txt"
#define SGIE3_CONFIG_FILE "dstest2_sgie3_config.txt"
#define MAX_DISPLAY_LEN 64

#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2


/* These are the strings of the labels for the respective models */
gchar sgie1_classes_str[12][32] = { "black", "blue", "brown", "gold", "green",
  "grey", "maroon", "orange", "red", "silver", "white", "yellow"
};

gchar sgie2_classes_str[20][32] =
    { "Acura", "Audi", "BMW", "Chevrolet", "Chrysler",
  "Dodge", "Ford", "GMC", "Honda", "Hyundai", "Infiniti", "Jeep", "Kia",
      "Lexus", "Mazda", "Mercedes", "Nissan",
  "Subaru", "Toyota", "Volkswagen"
};

gchar sgie3_classes_str[6][32] = { "coupe", "largevehicle", "sedan", "suv",
  "truck", "van"
};

gchar pgie_classes_str[4][32] =
    { "Vehicle", "TwoWheeler", "Person", "RoadSign" };

/* gie_unique_id is one of the properties in the above dstest2_sgiex_config.txt
 * files. These should be unique and known when we want to parse the Metadata
 * respective to the sgie labels. Ideally these should be read from the config
 * files but for brevity we ensure they are same. */

guint sgie1_unique_id = 2;
guint sgie2_unique_id = 3;
guint sgie3_unique_id = 4;

/* This is the buffer probe function that we have registered on the sink pad
 * of the OSD element. All the infer elements in the pipeline shall attach
 * their metadata to the GstBuffer, here we will iterate & process the metadata
 * forex: class ids to strings, counting of class_id objects etc. */
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
  gint sgie1_class_id = -1;
  NvDsAttrInfo *sgie1_attrs = NULL;
  gint sgie2_class_id = -1;
  NvDsAttrInfo *sgie2_attrs = NULL;
  gint sgie3_class_id = -1;
  NvDsAttrInfo *sgie3_attrs = NULL;
  NvOSD_TextParams *txt_params = NULL;
  guint tracking_id = 0;
  guint vehicle_count = 0;
  guint person_count = 0;
  static guint frame_number = 0;

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
          g_print ("NVDS Meta contained NULL meta \n");
          return GST_PAD_PROBE_OK;
        }

        /* We reset the num_strings here as we plan to iterate through the
         *  the detected objects and form our own strings by mapping the sgie
         *  class id label & its label string. The pipeline generated strings
         *  shall be discarded.
         */
        frame_meta->num_strings = 0;

        num_rects = frame_meta->num_rects;
        /* This means we have num_rects in frame_meta->obj_params,
         * now lets iterate through them */

        for (rect_index = 0; rect_index < num_rects; rect_index++) {

          /* Reset class string indexes per object detected */
          sgie1_class_id = -1;
          sgie2_class_id = -1;
          sgie3_class_id = -1;

          obj_meta = (NvDsObjectParams *) & frame_meta->obj_params[rect_index];

          if (obj_meta->class_id == PGIE_CLASS_ID_VEHICLE)
            vehicle_count++;
          if (obj_meta->class_id == PGIE_CLASS_ID_PERSON)
            person_count++;

          /* Now for each of the obj_meta, we iterate through the attr_infos.
           * gie_unique_id of the sgie instance should be known and it is
           * the index into the obj_meta->attr_info */
          sgie1_attrs = &(obj_meta->attr_info[sgie1_unique_id]);
          sgie2_attrs = &(obj_meta->attr_info[sgie2_unique_id]);
          sgie3_attrs = &(obj_meta->attr_info[sgie3_unique_id]);

          /* Our classifiers are single-class classifiers, hence num_attrs
           * should always be 1, for multi-class secondary classifiers, we
           * would have to iterate through the attr_info to get the respective
           * strings. */

          if (sgie1_attrs->num_attrs)
            sgie1_class_id = sgie1_attrs->attrs[0].attr_val;

          if (sgie2_attrs->num_attrs)
            sgie2_class_id = sgie2_attrs->attrs[0].attr_val;

          if (sgie3_attrs->num_attrs)
            sgie3_class_id = sgie3_attrs->attrs[0].attr_val;

          tracking_id = obj_meta->tracking_id;

          /* Now using above information we need to form a text that should
           * be displayed on top of the bounding box, so lets form it here.
           * We shall free any previously generated text by the pipeline, hence
           * free it first. */

          txt_params = &(obj_meta->text_params);
          if (txt_params->display_text)
            g_free (txt_params->display_text);

          txt_params->display_text = g_malloc0 (MAX_DISPLAY_LEN);

          g_snprintf (txt_params->display_text, MAX_DISPLAY_LEN, "%s ",
              pgie_classes_str[obj_meta->class_id]);

          if (sgie1_class_id != -1) {
            g_strlcat (txt_params->display_text, ": ", MAX_DISPLAY_LEN);
            g_strlcat (txt_params->display_text,
                sgie1_classes_str[sgie1_class_id], MAX_DISPLAY_LEN);
          }

          if (sgie2_class_id != -1) {
            g_strlcat (txt_params->display_text, ": ", MAX_DISPLAY_LEN);
            g_strlcat (txt_params->display_text,
                sgie2_classes_str[sgie2_class_id], MAX_DISPLAY_LEN);
          }

          if (sgie3_class_id != -1) {
            g_strlcat (txt_params->display_text, ": ", MAX_DISPLAY_LEN);
            g_strlcat (txt_params->display_text,
                sgie3_classes_str[sgie3_class_id], MAX_DISPLAY_LEN);
          }

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
        g_print ("Frame Number = %d Number of objects = %d"
            " Vehicle Count = %d Person Count = %d\n",
            frame_number, num_rects, vehicle_count, person_count);
      }
    }
  }
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
      *nvosd = NULL, *sgie1 = NULL, *sgie2 = NULL, *sgie3 = NULL,
      *nvtracker = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id = 0;
  GstPad *osd_sink_pad = NULL;
  gulong osd_probe_id = 0;

  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <elementary H264 filename>\n", argv[0]);
    return -1;
  }

  /* Standard GStreamer initialization */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create gstreamer elements */

  /* Create Pipeline element that will be a container of other elements */
  pipeline = gst_pipeline_new ("dstest2-pipeline");

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

  /* We need to have a tracker to track the identified objects */
  nvtracker = gst_element_factory_make ("nvtracker", "tracker");

  /* We need three secondary gies so lets create 3 more instances of
     nvinfer */
  sgie1 = gst_element_factory_make ("nvinfer", "secondary1-nvinference-engine");

  sgie2 = gst_element_factory_make ("nvinfer", "secondary2-nvinference-engine");

  sgie3 = gst_element_factory_make ("nvinfer", "secondary3-nvinference-engine");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv = gst_element_factory_make ("nvvidconv", "nvvideo-converter");

  /* Create OSD to draw on the converted RGBA buffer */
  nvosd = gst_element_factory_make ("nvosd", "nv-onscreendisplay");

  /* Finally render the osd output */
  sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");

  /* caps filter for nvvidconv to convert NV12 to RGBA as nvosd expects input
   * in RGBA format */
  if (!pipeline || !source || !h264parser || !decoder || !pgie || !nvtracker ||
      !sgie1 || !sgie2 || !sgie3 || !nvvidconv || !nvosd || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", argv[1], NULL);

  g_object_set (G_OBJECT (nvosd), "font-size", 15, NULL);

  /* Set all the necessary properties of the nvinfer element,
   * the necessary ones are : */
  g_object_set (G_OBJECT (pgie), "config-file-path", PGIE_CONFIG_FILE, NULL);
  g_object_set (G_OBJECT (sgie1), "config-file-path", SGIE1_CONFIG_FILE, NULL);
  g_object_set (G_OBJECT (sgie2), "config-file-path", SGIE2_CONFIG_FILE, NULL);
  g_object_set (G_OBJECT (sgie3), "config-file-path", SGIE3_CONFIG_FILE, NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);


  /* Set up the pipeline */
  /* we add all elements into the pipeline */
  /* decoder | pgie1 | nvtracker | sgie1 | sgie2 | sgie3 | etc.. */
  gst_bin_add_many (GST_BIN (pipeline),
      source, h264parser, decoder, pgie, nvtracker, sgie1, sgie2, sgie3,
      nvvidconv, nvosd, sink, NULL);

  /* Link the elements together */
  if (!gst_element_link_many (source, h264parser, decoder, pgie, nvtracker, sgie1,
      sgie2, sgie3, nvvidconv, nvosd, sink, NULL)) {
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

  /* Iterate */
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
