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

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "gstnvstreammeta.h"
#include "deepstream_perf.h"

#define TIMESPEC_DIFF_USEC(timespec1, timespec2) \
    (timespec1.tv_sec - timespec2.tv_sec) * 1000000.0 + \
    (timespec1.tv_nsec - timespec2.tv_nsec) / 1000.0

/**
 * Buffer probe function on sink element.
 */
static GstPadProbeReturn
sink_bin_buf_probe (GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
  NvDsAppPerfStructInt *str = (NvDsAppPerfStructInt *) u_data;
  GstNvStreamMeta *meta = NULL;
  guint i;

  if (!str->stop) {
    g_mutex_lock (&str->struct_lock);
    meta = gst_buffer_get_nvstream_meta(GST_BUFFER(info->data));

    for (i = 0; meta && i < meta->num_filled; i += str->dewarper_surfaces_per_frame) {
      NvDsInstancePerfStruct *str1 = &str->instance_str[meta->stream_id[i]];
      gettimeofday (&str1->last_fps_time, NULL);
      if (str1->start_fps_time.tv_sec == 0 &&
          str1->start_fps_time.tv_usec == 0) {
        str1->start_fps_time = str1->last_fps_time;
      } else {
        str1->buffer_cnt++;
      }
    }
    g_mutex_unlock (&str->struct_lock);
  }
  return GST_PAD_PROBE_OK;
}

static gboolean
perf_measurement_callback (gpointer data)
{
  NvDsAppPerfStructInt *str = (NvDsAppPerfStructInt *) data;
  guint buffer_cnt[MAX_SOURCE_BINS];
  NvDsAppPerfStruct perf_struct;
  guint i;

  g_mutex_lock (&str->struct_lock);
  if (str->stop) {
    g_mutex_unlock (&str->struct_lock);
    return FALSE;
  }

  for (i = 0; i < str->num_instances; i++) {
    buffer_cnt[i] = str->instance_str[i].buffer_cnt;
    str->instance_str[i].buffer_cnt = 0;
  }

  perf_struct.num_instances = str->num_instances;

  for (i = 0; i < str->num_instances; i++) {
    NvDsInstancePerfStruct *str1 = &str->instance_str[i];
    gdouble time1 =
        (str1->total_fps_time.tv_sec +
        str1->total_fps_time.tv_usec / 1000000.0) +
        (str1->last_fps_time.tv_sec + str1->last_fps_time.tv_usec / 1000000.0) -
        (str1->start_fps_time.tv_sec +
        str1->start_fps_time.tv_usec / 1000000.0);

    gdouble time2;

    if (str1->last_sample_fps_time.tv_sec == 0 &&
        str1->last_sample_fps_time.tv_usec == 0) {
      time2 =
          (str1->last_fps_time.tv_sec +
          str1->last_fps_time.tv_usec / 1000000.0) -
          (str1->start_fps_time.tv_sec +
          str1->start_fps_time.tv_usec / 1000000.0);
    } else {
      time2 =
          (str1->last_fps_time.tv_sec +
          str1->last_fps_time.tv_usec / 1000000.0) -
          (str1->last_sample_fps_time.tv_sec +
          str1->last_sample_fps_time.tv_usec / 1000000.0);
    }
    str1->total_buffer_cnt += buffer_cnt[i];
    perf_struct.fps[i] = buffer_cnt[i] / time2;
    if (isnan (perf_struct.fps[i]))
      perf_struct.fps[i] = 0;

    perf_struct.fps_avg[i] = str1->total_buffer_cnt / time1;
    if (isnan (perf_struct.fps_avg[i]))
      perf_struct.fps_avg[i] = 0;

    str1->last_sample_fps_time = str1->last_fps_time;
  }

  g_mutex_unlock (&str->struct_lock);

  str->callback (str->context, &perf_struct);

  return TRUE;
}

void
pause_perf_measurement (NvDsAppPerfStructInt *str)
{
  guint i;

  g_mutex_lock (&str->struct_lock);
  str->stop = TRUE;

  for (i = 0; i < str->num_instances; i++) {
    NvDsInstancePerfStruct *str1 = &str->instance_str[i];
    str1->total_fps_time.tv_sec +=
        str1->last_fps_time.tv_sec - str1->start_fps_time.tv_sec;
    str1->total_fps_time.tv_usec +=
        str1->last_fps_time.tv_usec - str1->start_fps_time.tv_usec;
    if (str1->total_fps_time.tv_usec < 0) {
      str1->total_fps_time.tv_sec--;
      str1->total_fps_time.tv_usec += 1000000;
    }
    str1->start_fps_time.tv_sec = str1->start_fps_time.tv_usec = 0;
  }

  g_mutex_unlock (&str->struct_lock);
}

void
resume_perf_measurement (NvDsAppPerfStructInt * str)
{
  guint i;

  g_mutex_lock (&str->struct_lock);
  if (!str->stop) {
    g_mutex_unlock (&str->struct_lock);
    return;
  }

  str->stop = FALSE;

  for (i = 0; i < str->num_instances; i++) {
    str->instance_str[i].buffer_cnt = 0;
  }

  str->perf_measurement_timeout_id =
      g_timeout_add (str->measurement_interval_ms, perf_measurement_callback,
      str);

  g_mutex_unlock (&str->struct_lock);
}

gboolean
enable_perf_measurement (NvDsAppPerfStructInt *str,
    GstPad *sink_bin_pad, guint num_sources,
    gulong interval_sec, perf_callback callback)
{
  guint i;

  if (!callback) {
    return FALSE;
  }

  str->num_instances = num_sources;

  str->measurement_interval_ms = interval_sec * 1000;
  str->callback = callback;
  str->stop = TRUE;
  if (str->dewarper_surfaces_per_frame == 0)
  {
    str->dewarper_surfaces_per_frame = 1;
  }

  for (i = 0; i < num_sources; i++) {
    str->instance_str[i].buffer_cnt = 0;
  }
  str->sink_bin_pad = sink_bin_pad;
  str->fps_measure_probe_id =
    gst_pad_add_probe (sink_bin_pad, GST_PAD_PROBE_TYPE_BUFFER,
        sink_bin_buf_probe, str, NULL);

  resume_perf_measurement (str);

  return TRUE;
}
