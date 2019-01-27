/**
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#ifndef __GST_DSEXAMPLE_H__
#define __GST_DSEXAMPLE_H__

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

/* Open CV headers */
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <cuda.h>
#include <cuda_runtime.h>
#include "nvbuffer.h"
#include "gst-nvquery.h"
#include "gstnvstreammeta.h"
#include "gstnvdsmeta.h"
#include "dsexample_lib/dsexample_lib.h"

/* Package and library details required for plugin_init */
#define PACKAGE "dsexample"
#define VERSION "1.0"
#define LICENSE "Proprietary"
#define DESCRIPTION "NVIDIA example plugin for integration with DeepStream on DGPU"
#define BINARY_PACKAGE "NVIDIA DeepStream 3rdparty IP integration example plugin"
#define URL "http://nvidia.com/"


G_BEGIN_DECLS
/* Standard boilerplate stuff */
typedef struct _GstDsExample GstDsExample;
typedef struct _GstDsExampleClass GstDsExampleClass;

/* Standard boilerplate stuff */
#define GST_TYPE_DSEXAMPLE (gst_dsexample_get_type())
#define GST_DSEXAMPLE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DSEXAMPLE,GstDsExample))
#define GST_DSEXAMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DSEXAMPLE,GstDsExampleClass))
#define GST_DSEXAMPLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_DSEXAMPLE, GstDsExampleClass))
#define GST_IS_DSEXAMPLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DSEXAMPLE))
#define GST_IS_DSEXAMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DSEXAMPLE))
#define GST_DSEXAMPLE_CAST(obj)  ((GstDsExample *)(obj))

struct _GstDsExample
{
  GstBaseTransform base_trans;

  // Context of the custom algorithm library
  DsExampleCtx *dsexamplelib_ctx;

  // Unique ID of the element. The labels generated by the element will be
  // updated at index `unique_id` of attr_info array in NvDsObjectParams.
  guint unique_id;

  // Frame number of the current input buffer
  guint64 frame_num;

  // NPP Stream used for allocating the CUDA task
  cudaStream_t npp_stream;

  // Host buffer to store RGB data for use by algorithm
  void *host_rgb_buf;

  // the intermediate scratch buffer for conversions
  void *inter_buf;

  // OpenCV mat containing RGB data
  cv::Mat *cvmat;

  // Input video info (resolution, color format, framerate, etc)
  GstVideoInfo video_info;

  // Resolution at which frames/objects should be processed
  gint processing_width;
  gint processing_height;

  // Amount of objects processed in single call to algorithm
  guint batch_size;

  // GPU ID on which we expect to execute the task
  guint gpu_id;

  // Boolean indicating if entire frame or cropped objects should be processed
  gboolean process_full_frame;
};

// Boiler plate stuff
struct _GstDsExampleClass
{
  GstBaseTransformClass parent_class;
};

GType gst_dsexample_get_type (void);

G_END_DECLS
#endif /* __GST_DSEXAMPLE_H__ */
