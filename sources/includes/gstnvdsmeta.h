/*
 * Copyright (c) 2015-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

/**
 * @file
 * <b>NVIDIA GStreamer DeepStream: Metadata Extension</b>
 *
 * @b Description: This file specifies the NVIDIA DeepStream Metadata structures used to
 * describe metadata objects.
 */

/**
 * @defgroup gstreamer_metagroup_api DeepStream Metadata Extension
 *
 * Defines an API for managing GStreamer DeepStream metadata.
 * @ingroup gstreamer_metadata_group
 * @{
 *
 * DeepStream Metadata is attached to a buffer with gst_buffer_add_nvds_meta().
 *
 * Multiple metadatas may be attached by different elements.
 * gst_buffer_get_nvds_meta() gets the last added @ref NvDsMeta.
 * To iterate through each NvDsMeta, following snippet can be used.
 *
 * @code
 *  gpointer state = NULL;
 *  GstMeta *gst_meta;
 *  NvDsMeta *nvdsmeta;
 *  static GQuark _nvdsmeta_quark = 0;
 *
 *  if (!_nvdsmeta_quark)
 *    _nvdsmeta_quark = g_quark_from_static_string(NVDS_META_STRING);
 *
 *  while ((gst_meta = gst_buffer_iterate_meta (buf, &state))) {
 *    if (gst_meta_api_type_has_tag(gst_meta->info->api, _nvdsmeta_quark)) {
 *      nvdsmeta = (NvDsMeta *) gst_meta;
 *      // Do something with this nvdsmeta
 *    }
 *  }
 * @endcode
 *
 *  The meta_data member of the NvDsMeta structure must be cast to a meaningful structure pointer
 *  based on the meta_type. For example, for `meta_type = NVDS_META_FRAME_INFO`,
 *  meta_data must be cast as `(NvDsFrameMeta *)`.
 */

#ifndef GST_NVDS_META_API_H
#define GST_NVDS_META_API_H

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/base/gstbasetransform.h>

#include "nvdsmeta.h"
#include "nvosd.h"

#ifdef __cplusplus
extern "C"
{
#endif
GType nvds_meta_api_get_type (void);
#define NVDS_META_API_TYPE (nvds_meta_api_get_type())

const GstMetaInfo *nvds_meta_get_info (void);
#define NVDS_META_INFO (nvds_meta_get_info())

#define NVDS_MAX_ATTRIBUTES 16

#define NVDS_META_STRING "nvdsmeta"

/**
 * Specifies the type of function to copy meta data.
 * It is passed the pointer to meta data and user specific data.
 * It allocates the required memory, copy the content and returns
 * the pointer to newly allocated memory.
 */
typedef gpointer (*NvDsMetaCopyFunc) (gpointer data, gpointer user_data);

/**
 * Specifies the type of function to free meta data.
 * It is passed the pointer to meta data and user specific data.
 * It should free any resource allocated for meta data.
 */
typedef void (*NvDsMetaFreeFunc) (gpointer data, gpointer user_data);

/** Defines DeepStream meta data types. */
typedef enum
{
    NVDS_META_INVALID=-1,
    /** Indicates that the meta data contains objects information (NvDsFrameMeta) */
    NVDS_META_FRAME_INFO = 0x01,
    /** Indicates that the meta data contains lines information for the given stream (NvDsLine_Params) */
    NVDS_META_LINE_INFO = 0x02,
    /** Payload for backend server as meta data */
    NVDS_META_PAYLOAD,
    /** event messages as meta data */
    NVDS_META_EVENT_MSG,
    NVDS_META_RESERVED = 0x100,
    /** Start adding custom meta types from here */
    NVDS_META_CUSTOM = 0x101,
    NVDS_META_FORCE32 = 0x7FFFFFFF
} NvDsMetaType;

/** Defines DeepStream meta surface types. */
typedef enum
{
    NVDS_META_SURFACE_NONE=0,
    /** Indicates that the meta data contains surface type */
    NVDS_META_SURFACE_FISH_PUSHBROOM=1,
    NVDS_META_SURFACE_FISH_VERTCYL=2,
} NvDsSurfaceType;

/**
 * Holds information about one secondary label attribute
 */
typedef struct _NvDsAttr {
  /** Attribute id */
  gshort attr_id;
  /** Attribute value */
  gshort attr_val;
  /** Attribute probability. This will have float value between 0 to 1 */
  gfloat attr_prob;
} NvDsAttr;

/** Holds data that secondary classifiers / custom elements update with
 *  secondary label information, such as car type, car color, etc.
 *  The structure may contain a string label. It may also contain a set of
 *  N(num_attrs) pairs of `<attr_id,attr_val>`.
 *
 *  For single label classifiers, `attr_id` will always be 0, and N=1.
 *  For multi-label classifiers, `attr_id` will be the index of the attribute
 *  type (e.g. 0 for Car type, 1 for Car make, ...).
 *  In both cases, `attr_val` will be the value of the attribute
 *  (e.g. 0 for Sedan, 1 for Coupe, ...)
 */
typedef struct _NvDsAttrInfo {
  /** Boolean indicating whether @a attr_label is valid. */
  gboolean is_attr_label;
  /** String label */
  gchar attr_label[64];
  /** Number of valid elements in the @a attrs array. */
  gint num_attrs;
  /** An array of attribute id and value, only @a num_attrs elements are valid. */
  NvDsAttr attrs[64];
} NvDsAttrInfo;

/** Holds extended parameters that describe meta data for the object. */
typedef struct _NvDsObjectParamsEx {
  gint param1;
  gint param2;
  gint param3;
  gint param4;
} NvDsObjectParamsEx;

/** Holds parameters that describe meta data for one object. */
typedef struct _NvDsObjectParams {
  /** Structure containing the positional parameters of the object in the frame.
   *  Can also be used to overlay borders / semi-transparent boxes on top of objects
   *  Refer NvOSD_RectParams from nvosd.h
   */
  NvOSD_RectParams rect_params;
  /** Text describing the object can be overlayed using this structure.
   *  @see NvOSD_TextParams from nvosd.h. */
  NvOSD_TextParams text_params;
  /** Index of the object class infered by the primary detector/classifier */
  gint class_id;
  /** Unique ID for tracking the object. This -1 indicates the object has not been
   * tracked. Custom elements adding new NvDsObjectParams should set this to
   * -1. */
  gint tracking_id;
  /** Secondary classifiers / custom elements update this structure with
   *  secondary classification labels. Each element will only update the
   *  attr_info structure at index specified by the element's unique id.
   */
  NvDsAttrInfo attr_info[NVDS_MAX_ATTRIBUTES];
  /** Boolean indicating whether attr_info contains new information. */
  gboolean has_new_info;
  /** Boolean indicating whether secondary classifiers should run inference on the object again. Used internally by components. */
  gboolean force_reinference;
  /** Used internally by components. */
  gint parent_class_id;
  /** Used internally by components. */
  struct _NvDsObjectParams *parent_roi;
  /** Used internally by components */
  NvOSD_LineParams line_params[4];
  /** Used internally by components */
  gint lines_present;
  /** Used internally by components */
  gint obj_status;
  /** Provision to add custom object metadata info */
  NvDsObjectParamsEx obj_params_ex;
} NvDsObjectParams;

/** Holds data that describes metadata objects in the current frame.
    `meta_type` member of @ref NvDsMeta must be set to `NVDS_META_FRAME_INFO`. */
typedef struct _NvDsFrameMeta {
  /** Array of NvDsObjectParams structure describing each object. */
  NvDsObjectParams *obj_params;
  /** Number of rectangles/objects i.e. length of @ref NvDsObjectParams */
  guint num_rects;
  /** Number of valid strings in @ref NvDsObjectParams. */
  guint num_strings;
  /** Index of the frame in the batched buffer to which this meta belongs to. */
  guint batch_id;
  /** NvOSD mode to be used. @see NvOSD_Mode in `nvosd.h`. */
  NvOSD_Mode nvosd_mode;
  /** 1 = Primary GIE, 2 = Secondary GIE, 3 = Custom Elements */
  gint gie_type;
  /** Batch size of the primary detector. */
  gint gie_batch_size;
  /** Unique ID of the primary detector that attached this metadata. */
  gint gie_unique_id;
  /** Frame number. */
  gint frame_num;
  /** Index of the stream this params structure belongs to. */
  guint stream_id;
  /** Boolean indicating if these params are no longer valid. */
  gint is_invalid;
  /** Indicates Surface Type i.e. Spot or Aisle */
  NvDsSurfaceType surface_type;
  /** Indicates Surface Index for SurfaceType Spot or Aisle */
  gint camera_id;
  /** Indicates Surface Index for SurfaceType Spot or Aisle */
  gint surface_index;
} NvDsFrameMeta;

/** Holds line meta data. */
typedef struct _NvDsLineMeta {
  /** Index of the frame in the batched buffer to which this meta belongs to. */
  guint batch_id;
  /** number of lines to be drawn i.e. valid length of @ref NvDsLineMeta . */
  guint num_lines;
  /** Array of NvDsLineMeta structure describing each line. */
  NvOSD_LineParams *line_params;
} NvDsLineMeta;

/** Holds DeepSteam meta data. */
 typedef struct _NvDsMeta {
  GstMeta       meta;
  /** Must be cast to another structure based on @a meta_type. */
  gpointer meta_data;
  gpointer user_data;
  /** Type of metadata, from the @ref meta_type enum. */
  gint meta_type;
  /** Function called with meta_data pointer as argument when the meta is going to be destroyed.
   * Can be used to clear/free meta_data.
   * Refer to https://developer.gnome.org/glib/unstable/glib-Datasets.html#GDestroyNotify */
  GDestroyNotify destroy;
  /**
   * It is called when meta_data needs to copied / transformed
   * from one buffer to other. meta_data and user_data are passed as arguments.
   */
  NvDsMetaCopyFunc copyfunc;
  /**
   * It is called when meta_data is going to be destroyed.
   * Both destroy or freefunc must not be set. User freefunc only if
   * GDestroyNotify is not sufficient to release the resources.
   * meta_data and user_data are passed as arguments.
   */
  NvDsMetaFreeFunc freefunc;
} NvDsMeta;

/**
 * Adds GstMeta of type @ref NvDsMeta to the GstBuffer and sets the `meta_data` member of @ref NvDsMeta.
 *
 * @param[in] buffer GstBuffer to which the function adds metadata.
 * @param[in] meta_data The pointer to which the function sets the meta_data member of @ref NvDsMeta.
 * @param[in] destroy The GDestroyNotify function to be called when NvDsMeta is to be destroyed.
 *            The function is called with meta_data as a parameter. Refer to
 *            https://developer.gnome.org/glib/unstable/glib-Datasets.html#GDestroyNotify.
 *
 * @return A pointer to the attached @ref NvDsMeta structure; or NULL in case of failure.
 */
NvDsMeta* gst_buffer_add_nvds_meta (GstBuffer *buffer, gpointer meta_data,
        GDestroyNotify destroy);

/** Gets the @ref NvDsMeta last added to the GstBuffer.
 *
 * @param[in] buffer GstBuffer
 *
 * @return A pointer to the last added NvDsMeta structure; or NULL if no NvDsMeta was attached.
 */
NvDsMeta* gst_buffer_get_nvds_meta (GstBuffer *buffer);

/**
 * Calls nvds_meta_set_copy_function_full with NULL for user_data and free_func
 * This should be used when no user specific data is needed for copy and
 * GDestroyNotify is sufficient to release the resource.
 *
 * @param[in] meta A pointer to @ref NvDsMeta.
 * @param[in] func @ref NvDsMetaCopyFunc function to set.
 */
void nvds_meta_set_copy_function (NvDsMeta *meta, NvDsMetaCopyFunc func);

/**
 * Sets the given function to copy meta data. This function will be used
 * when meta data is copied / transformed from one buffer to other.
 *
 * @param[in] meta pointer to @ref NvDsMeta.
 * @param[in] copy_func @ref NvDsMetaCopyFunc function to set.
 * @param[in] user_data pointer to user data that is passed to copy / free functions.
 * @param[in] free_func @ref NvDsMetaFreeFunc function to release the meta data.
 */
void nvds_meta_set_copy_function_full (NvDsMeta *meta, NvDsMetaCopyFunc copy_func,
                                       gpointer user_data, NvDsMetaFreeFunc free_func);

/** Creates a deep copy of the input NvDsFrameMeta structure.
 *
 * @param[in] src Pointer to the input NvDsFrameMeta structure to be copied.
 *
 * @return A pointer to the copied NvDsFrameMeta structure.
 */
NvDsFrameMeta* nvds_copy_frame_meta (NvDsFrameMeta *src);
/**
 * Frees a heap-allocated NvDsFrameMeta structure, including members
 * that were heap-allocated.
 *
 * @param[in] params Pointer to the heap-allocated NvDsFrameMeta structure.
 */
void nvds_free_frame_meta (NvDsFrameMeta *params);

/** Creates a deep copy of the input NvDsLineMeta structure.
 *
 * @param[in] src Pointer to the input NvDsLineMeta structure to be copied.
 *
 * @return A pointer to the copied NvDsLineMeta structure.
 */
NvDsLineMeta * nvds_copy_line_info (NvDsLineMeta * src);

/**
 * Frees a heap-allocated NvDsLineMeta structure, including members
 * that were heap-allocated.
 *
 * @param[in] meta Pointer to the heap-allocated NvDsLineMeta structure.
 */
void nvds_free_line_info (gpointer meta);

/** @} */
#ifdef __cplusplus
}
#endif
#endif
