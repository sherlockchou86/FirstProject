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

#ifndef __NVGSTDS_PERF_H__
#define __NVGSTDS_PERF_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>
#include "deepstream_config.h"

typedef struct
{
  gdouble fps[MAX_SOURCE_BINS];
  gdouble fps_avg[MAX_SOURCE_BINS];
  guint num_instances;
  gdouble cpu_usage;
  gdouble cpu_usage_avg;
  gdouble gpu_usage;
  gdouble gpu_usage_avg;
  gulong cpu_freq_mhz;
  gulong gpu_freq_mhz;  
} NvDsAppPerfStruct;

typedef void (*perf_callback) (gpointer ctx, NvDsAppPerfStruct * str);

typedef struct
{
  guint buffer_cnt;
  guint total_buffer_cnt;
  struct timeval total_fps_time;
  struct timeval start_fps_time;
  struct timeval last_fps_time;
  struct timeval last_sample_fps_time;
} NvDsInstancePerfStruct;

typedef struct
{
  gulong measurement_interval_ms;
  gulong perf_measurement_timeout_id;
  guint num_instances;
  gboolean stop;
  gpointer context;
  GMutex struct_lock;
  perf_callback callback;
  GstPad *sink_bin_pad;
  gulong fps_measure_probe_id;
  NvDsInstancePerfStruct instance_str[MAX_SOURCE_BINS];
  guint dewarper_surfaces_per_frame;
} NvDsAppPerfStructInt;

gboolean enable_perf_measurement (NvDsAppPerfStructInt *str,
    GstPad *sink_bin_pad, guint num_sources, gulong interval_sec,
    perf_callback callback);

void pause_perf_measurement (NvDsAppPerfStructInt *str);
void resume_perf_measurement (NvDsAppPerfStructInt *str);

#ifdef __cplusplus
}
#endif

#endif
