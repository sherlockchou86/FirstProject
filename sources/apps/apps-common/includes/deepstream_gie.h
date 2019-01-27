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

#ifndef __NVGSTDS_GIE_H__
#define __NVGSTDS_GIE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>
#include "gstnvdsmeta.h"
#include "gstnvdsinfer.h"
#include "deepstream_config.h"

//CHWO --> channel, Height, width, Input Order (CHW(0) or HWC(1)) --> 3;224;224;0
#define NETWORK_INPUT_SIZE 4

typedef struct
{
  gboolean enable;

  gchar *config_file_path;

  gboolean override_colors;

  gint operate_on_gie_id;
  gboolean is_operate_on_gie_id_set;
  gint operate_on_classes;

  gint num_operate_on_class_ids;
  gint *list_operate_on_class_ids;

  gboolean have_bg_color;
  NvOSD_ColorParams bbox_bg_color;
  NvOSD_ColorParams bbox_border_color;

  GHashTable *bbox_border_color_table;
  GHashTable *bbox_bg_color_table;

  guint batch_size;
  gboolean is_batch_size_set;

  guint interval;
  gboolean is_interval_set;
  guint unique_id;
  gboolean is_unique_id_set;
  guint gpu_id;
  gboolean is_gpu_id_set;
  guint cuda_memory_type;
  gchar *model_engine_file_path;

  gchar *label_file_path;
  guint n_labels;
  guint *n_label_outputs;
  gchar ***labels;

  gchar *raw_output_directory;
  gulong file_write_frame_num;

  gchar *tag;
} NvDsGieConfig;

#ifdef __cplusplus
}
#endif

#endif
