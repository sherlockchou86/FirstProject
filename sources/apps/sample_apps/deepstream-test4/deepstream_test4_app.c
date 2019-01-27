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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include "gstnvdsmeta.h"

#define MAX_DISPLAY_LEN 64
#define MAX_TIME_STAMP_LEN 32

#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2

#define PROTOCOL_ADAPTOR_LIB  "libnvds_kafka_proto.so"
#define CONNECTION_STRING "foo.bar.com;80;dsapp1"

gint frame_number = 0;
gchar pgie_classes_str[4][32] = { "Vehicle", "TwoWheeler", "Person",
  "Roadsign"
};

static void generate_ts_rfc3339 (char *buf, int buf_size)
{
  time_t tloc;
  struct timeb timeb_log;
  struct tm tm_log;
  struct timespec ts;
  char strmsec[6]; //.nnnZ\0

  clock_gettime(CLOCK_REALTIME,  &ts);
  memcpy(&tloc, (void *)(&ts.tv_sec), sizeof(time_t));
  gmtime_r(&tloc, &tm_log);
  strftime(buf, buf_size,"%Y-%m-%dT%H:%M:%S", &tm_log);
  int ms = ts.tv_nsec/1000000;
  g_snprintf(strmsec, sizeof(strmsec),".%.3dZ", ms);
  strncat(buf, strmsec, buf_size);
}

static gpointer meta_copy_func (gpointer data, gpointer user_data)
{
  NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *) data;
  NvDsEventMsgMeta *dstMeta = NULL;

  dstMeta = g_memdup (srcMeta, sizeof(NvDsEventMsgMeta));

  if (srcMeta->ts)
    dstMeta->ts = g_strdup (srcMeta->ts);

  if (srcMeta->objSignature.size > 0) {
    dstMeta->objSignature.signature = g_memdup (srcMeta->objSignature.signature,
                                                srcMeta->objSignature.size);
    dstMeta->objSignature.size = srcMeta->objSignature.size;
  }

  if (srcMeta->extMsgSize > 0) {
    if (srcMeta->objType == NVDS_OBJECT_TYPE_VEHICLE) {
      NvDsVehicleObject *srcObj = (NvDsVehicleObject *) srcMeta->extMsg;
      NvDsVehicleObject *obj = (NvDsVehicleObject *) g_malloc0 (sizeof (NvDsVehicleObject));
      if (srcObj->type)
        obj->type = g_strdup (srcObj->type);
      if (srcObj->make)
        obj->make = g_strdup (srcObj->make);
      if (srcObj->model)
        obj->model = g_strdup (srcObj->model);
      if (srcObj->color)
        obj->color = g_strdup (srcObj->color);
      if (srcObj->license)
        obj->license = g_strdup (srcObj->license);
      if (srcObj->region)
        obj->region = g_strdup (srcObj->region);

      dstMeta->extMsg = obj;
      dstMeta->extMsgSize = sizeof (NvDsVehicleObject);
    } else if (srcMeta->objType == NVDS_OBJECT_TYPE_PERSON) {
      NvDsPersonObject *srcObj = (NvDsPersonObject *) srcMeta->extMsg;
      NvDsPersonObject *obj = (NvDsPersonObject *) g_malloc0 (sizeof (NvDsPersonObject));

      obj->age = srcObj->age;

      if (srcObj->gender)
        obj->gender = g_strdup (srcObj->gender);
      if (srcObj->cap)
        obj->cap = g_strdup (srcObj->cap);
      if (srcObj->hair)
        obj->hair = g_strdup (srcObj->hair);
      if (srcObj->apparel)
        obj->apparel = g_strdup (srcObj->apparel);
    }
  }

  return dstMeta;
}

static void meta_free_func (gpointer data, gpointer user_data)
{
  NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *) data;

  if (srcMeta->ts)
    g_free (srcMeta->ts);

  if (srcMeta->objSignature.size > 0) {
    g_free (srcMeta->objSignature.signature);
    srcMeta->objSignature.size = 0;
  }

  if (srcMeta->extMsgSize > 0) {
    if (srcMeta->objType == NVDS_OBJECT_TYPE_VEHICLE) {
      NvDsVehicleObject *obj = (NvDsVehicleObject *) srcMeta->extMsg;
      if (obj->type)
        g_free (obj->type);
      if (obj->color)
        g_free (obj->color);
      if (obj->make)
        g_free (obj->make);
      if (obj->model)
        g_free (obj->model);
      if (obj->license)
        g_free (obj->license);
      if (obj->region)
        g_free (obj->region);
    } else if (srcMeta->objType == NVDS_OBJECT_TYPE_PERSON) {
      NvDsPersonObject *obj = (NvDsPersonObject *) srcMeta->extMsg;

      if (obj->gender)
        g_free (obj->gender);
      if (obj->cap)
        g_free (obj->cap);
      if (obj->hair)
        g_free (obj->hair);
      if (obj->apparel)
        g_free (obj->apparel);
    }
    g_free (srcMeta->extMsg);
    srcMeta->extMsgSize = 0;
  }
  g_free (data);
}

static void
generate_vehicle_meta (gpointer data)
{
  NvDsVehicleObject *obj = (NvDsVehicleObject *) data;

  obj->type = g_strdup ("sedan");
  obj->color = g_strdup ("blue");
  obj->make = g_strdup ("Bugatti");
  obj->model = g_strdup ("M");
  obj->license = g_strdup ("XX1234");
  obj->region = g_strdup ("CA");
}

static void
generate_person_meta (gpointer data)
{
  NvDsPersonObject *obj = (NvDsPersonObject *) data;
  obj->age = 45;
  obj->cap = g_strdup ("none");
  obj->hair = g_strdup ("black");
  obj->gender = g_strdup ("male");
  obj->apparel= g_strdup ("formal");
}

static void
generate_event_msg_meta (gpointer data, gint class_id)
{
  NvDsEventMsgMeta *meta = (NvDsEventMsgMeta *) data;
  meta->sensorId = 0;
  meta->placeId = 0;
  meta->moduleId = 0;

  meta->ts = (gchar *) g_malloc0 (MAX_TIME_STAMP_LEN + 1);

  generate_ts_rfc3339(meta->ts, MAX_TIME_STAMP_LEN);

  /*
   * This demonstrates how to attach custom objects.
   * Any custom object as per requirement can be generated and attached
   * like NvDsVehicleObject / NvDsPersonObject. Then that object should
   * be handled in gst-nvmsgconv component accordingly.
   */
  if (class_id == PGIE_CLASS_ID_VEHICLE) {
    meta->type = NVDS_EVENT_MOVING;
    meta->objType = NVDS_OBJECT_TYPE_VEHICLE;
    meta->objClassId = PGIE_CLASS_ID_VEHICLE;

    NvDsVehicleObject *obj = (NvDsVehicleObject *) g_malloc0 (sizeof (NvDsVehicleObject));
    generate_vehicle_meta (obj);

    meta->extMsg = obj;
    meta->extMsgSize = sizeof (NvDsVehicleObject);
  } else if (class_id == PGIE_CLASS_ID_PERSON) {
    meta->type = NVDS_EVENT_ENTRY;
    meta->objType = NVDS_OBJECT_TYPE_PERSON;
    meta->objClassId = PGIE_CLASS_ID_PERSON;

    NvDsPersonObject *obj = (NvDsPersonObject *) g_malloc0 (sizeof (NvDsPersonObject));
    generate_person_meta (obj);

    meta->extMsg = obj;
    meta->extMsgSize = sizeof (NvDsPersonObject);
  }
}

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
  gboolean is_first_rect = TRUE;

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
        is_first_rect = TRUE;

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

          /*
           * Ideally NVDS_META_EVENT_MSG should be attached to buffer by the
           * component implementing detection / recognition logic.
           * Here it demonstrates how to use / attach that meta data.
           */
          if (is_first_rect && !(frame_number % 30)) {
            /* Frequency of messages to be send will be based on use case.
             * Here message is being sent for first object every 30 frames.
             */
            NvDsMeta *gst_event_meta = NULL;
            NvDsEventMsgMeta *msg_meta = (NvDsEventMsgMeta *) g_malloc0 (sizeof (NvDsEventMsgMeta));
            generate_event_msg_meta (msg_meta, obj_meta->class_id);
            gst_event_meta = gst_buffer_add_nvds_meta (buf, msg_meta, NULL);
            if (gst_event_meta) {
              gst_event_meta->meta_type = NVDS_META_EVENT_MSG;
              /*
               * Since generated event metadata has custom objects for
               * Vehicle / Person which are allocated dynamically, we are
               * setting copy and free function to handle those fields when
               * metadata copy happens between two components.
               */
              nvds_meta_set_copy_function_full(gst_event_meta, meta_copy_func, NULL, meta_free_func);
            } else {
              g_print ("Error in attaching event meta to buffer\n");
            }
            is_first_rect = FALSE;
          }
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
  GstElement *msgconv = NULL, *msgbroker = NULL, *tee = NULL;
  GstElement *queue1 = NULL, *queue2 = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id;
  gulong osd_probe_id = 0;
  GstPad *osd_sink_pad = NULL;
  GstPad *tee_render_pad = NULL;
  GstPad *tee_msg_pad = NULL;
  GstPad *sink_pad = NULL;

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
  pipeline = gst_pipeline_new ("dstest4-pipeline");

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

  /* Create msg converter to generate payload from buffer metadata */
  msgconv = gst_element_factory_make ("nvmsgconv", "nvmsg-converter");

  /* Create msg broker to send payload to server */
  msgbroker = gst_element_factory_make ("nvmsgbroker", "nvmsg-broker");

  /* Create tee to render buffer and send message simultaneously*/
  tee = gst_element_factory_make ("tee", "nvsink-tee");

  /* Create queues */
  queue1 = gst_element_factory_make ("queue", "nvtee-que1");
  queue2 = gst_element_factory_make ("queue", "nvtee-que2");

  /* Finally render the osd output */
  sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");

  if (!pipeline || !source || !h264parser || !decoder || !pgie
      || !nvvidconv || !nvosd || !msgconv || !msgbroker || !tee
      || !queue1 || !queue2 || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* we set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", argv[1], NULL);

  /* Set all the necessary properties of the nvinfer element,
   * the necessary ones are : */
  g_object_set (G_OBJECT (pgie),
      "config-file-path", "dstest4_pgie_config.txt", NULL);

  /* we set the osd properties here */
  g_object_set (G_OBJECT (nvosd), "font-size", 15, NULL);

  g_object_set (G_OBJECT(msgconv), "config", "dstest4_msgconv_config.txt", NULL);

  g_object_set (G_OBJECT(msgbroker), "proto-lib", PROTOCOL_ADAPTOR_LIB,
                "conn-str", CONNECTION_STRING, "sync", FALSE, NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Set up the pipeline */
  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline),
      source, h264parser, decoder, pgie,
      nvvidconv, nvosd, tee, queue1, queue2, msgconv,
      msgbroker, sink, NULL);

  /* we link the elements together */
  /* file-source -> h264-parser -> nvh264-decoder ->
   * nvinfer -> nvvidconv -> nvosd -> tee -> video-renderer
   *                                      |
   *                                      |-> msgconv -> msgbroker  */

  if (!gst_element_link_many (source, h264parser, decoder, pgie,
      nvvidconv, nvosd, tee, NULL)) {
    g_printerr ("Elements could not be linked. Exiting.\n");
    return -1;
  }

  if (!gst_element_link_many (queue1, msgconv, msgbroker, NULL)) {
    g_printerr ("Elements could not be linked. Exiting.\n");
    return -1;
  }

  if (!gst_element_link (queue2, sink)) {
    g_printerr ("Elements could not be linked. Exiting.\n");
    return -1;
  }

  sink_pad = gst_element_get_static_pad (queue1, "sink");
  tee_msg_pad = gst_element_get_request_pad (tee, "src_%u");
  tee_render_pad = gst_element_get_request_pad (tee, "src_%u");
  if (!tee_msg_pad || !tee_render_pad) {
    g_printerr ("Unable to get request pads\n");
    return -1;
  }

  if (gst_pad_link (tee_msg_pad, sink_pad) != GST_PAD_LINK_OK) {
    g_printerr ("Unable to link tee and message converter\n");
    gst_object_unref (sink_pad);
    return -1;
  }

  gst_object_unref (sink_pad);

  sink_pad = gst_element_get_static_pad (queue2, "sink");
  if (gst_pad_link (tee_render_pad, sink_pad) != GST_PAD_LINK_OK) {
    g_printerr ("Unable to link tee and render\n");
    gst_object_unref (sink_pad);
    return -1;
  }

  gst_object_unref (sink_pad);

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

  /* Release the request pads from the tee, and unref them */
  gst_element_release_request_pad (tee, tee_msg_pad);
  gst_element_release_request_pad (tee, tee_render_pad);
  gst_object_unref (tee_msg_pad);
  gst_object_unref (tee_render_pad);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  return 0;
}
