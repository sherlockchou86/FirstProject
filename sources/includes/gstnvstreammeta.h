/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA GStreamer DeepStream: Multi-Stream Metadata</b>
 *
 * @b Description: This file specifies the NVIDIA DeepStream GStreamer
 * Multi-Stream Metadata format.
 */

/**
 * @defgroup gst_metadata_plugin Multi-Stream Metadata Functions
 * Defines the GStreamer multi-stream metadata format.
 * The Multi-Stream Metadata is attached to a batched GStreamer buffer by the
 * Stream Muxer which creates the batched buffer. The metadata contains
 * information related to the original buffers used to form the batched buffer.
 * @ingroup gstreamer_metadata_group
 * @{
 */


#ifndef __GST_NVSTREAM_META_H__
#define __GST_NVSTREAM_META_H__

#include <gst/gst.h>
#include "gstnvdsmeta.h"

G_BEGIN_DECLS
#define GST_META_TAG_NVSTREAM "nvstream"
#define GST_NVSTREAM_META_API_TYPE (gst_nvstream_meta_api_get_type())
#define GST_NVSTREAM_META_INFO  (gst_nvstream_meta_get_info())
#define GST_CAPS_FEATURE_META_GST_NVSTREAM_META "meta:GstNvStreamMeta"

/**
 * Holds information related to the original buffers contained in a batched
 * buffer.
 */
typedef struct
{
  /** Parent GstMeta structure. */
  GstMeta meta;
  /** Number of actual filled frames in a batch. This number may be less than
   * the batch size of the buffer. */
  guint num_filled;

  /** Array of indexes of stream to which the frames in the batch belong to. */
  guint *stream_id;
  /** Array of frame numbers of the frames in the batch. The frame numbers are
   * the numbers of the frame in the input stream. */
  gulong *stream_frame_num;
  /** Array of original input widths of the frames in the batch. This might be
   * different than the width of the batched frame since the Stream Muxer might
   * scale the frame during batching. */
  guint *original_width;
  /** Array of original input heights of the frames in the batch. This might be
   * different than the width of the batched frame since the Stream Muxer might
   * scale the frame during batching. */
  guint *original_height;
  /** Array of original presentation timestamps of the frames in the batch.
   * Stream Muxer will attach its own timestamp to the batched GstBuffer. */
  GstClockTime *buf_pts;

  /** Array of Camera IDs indicating the camera_id which maps to CSV file */
  guint *camera_id;
  /** Total number of dewarped surfaces per source frame */
  guint num_surfaces_per_frame;
  /** Total batch-size */
  guint batch_size;
  /** Array of Surface Types Spot / Aisle / None */
  NvDsSurfaceType *surface_type;
  /** Array of surface_index indicating the surface index which maps to CSV file */
  guint *surface_index;

  /** Used internally by components. */
  gchar **input_pkt_pts;
  /** Used internally by components. */
  gboolean *is_valid;
} GstNvStreamMeta;

GType gst_nvstream_meta_api_get_type (void);
const GstMetaInfo *gst_nvstream_meta_get_info (void);

/**
 * Get NvStream meta attached to a GstBuffer.
 *
 * params[in] buffer Pointer to a GstBuffer
 *
 * @returns Pointer to a valid GstNvStreamMeta, NULL if the GstBuffer does not
 * have a GstNvStreamMeta attached.
 */
GstNvStreamMeta *gst_buffer_get_nvstream_meta (GstBuffer * buffer);

/**
 * Add GstNvStream meta to a GstBuffer.
 *
 * params[in] buffer Pointer to a GstBuffer
 * params[in] batch_size Batch size of the buffer
 *
 * @returns Pointer to a valid GstNvStreamMeta, NULL if the meta could not be
 * added.
 */
GstNvStreamMeta *gst_buffer_add_nvstream_meta (GstBuffer * buffer, guint batch_size);

/** @} */

G_END_DECLS
#endif
