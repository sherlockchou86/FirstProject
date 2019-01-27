/* Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA DeepStream: Buffer Structure</b>
 *
 * @b Description: This file specifies the NVIDIA DeepStream GStreamer batched
 * buffer structure.
 */

/**
 * @defgroup gstreamer_buffer Buffer Structures
 * Defines buffer structures.
 * @ingroup gstreamer_utilities_group
 * @{
 *
 * Clients use the @ref NvBufSurface structure to access the
 * buffers that DeepStream GStreamer components allocate.
 * Additionally, they can use it to access the related
 * buffer format information.
 *
 * To get NvBufSurface structure from a DeepStream GStreamer buffer,
 * use the following snippet:
 *
 * @code
 *  GstMapInfo info = GST_MAP_INFO_INIT;
 *  NvBufSurface *nvbuf = NULL;
 *
 *  if (!gst_buffer_map (gstbuf, &info, GST_MAP_READ)) {
 *    goto map_error;
 *  }
 *
 *  nvbuf = (NvBufSurface *) info.data;
 *
 *  gst_buffer_unmap (gstbuf, &info);
 *
 * map_error:
 *  // Handle buffer mapping error
 *
 * @endcode
 *
 * This GStreamer buffer contains a batch of one or more video frames from one
 * or more input sources. The batched buffer is basically contiguous memory
 * allocated to accomodate N video frame buffers.
 */

#ifndef _NVBUFFER_H_
#define _NVBUFFER_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** Defines the maximum number of buffers that the batched buffer can contain. */
#define MAX_NVBUFFERS 1024

#define CHECK_NVDS_MEMORY_AND_GPUID(object, surface)  \
  ({ int _errtype=0;\
   do {  \
    if ((surface->mem_type == CUDA_DEVICE_MEMORY) && \
        (surface->gpu_id != object->gpu_id))  { \
    GST_ELEMENT_ERROR (object, RESOURCE, FAILED, \
        ("Input surface gpu-id doesnt match with configured gpu-id for element," \
         " please allocate input using unified memory, or use same gpu-ids"),\
        ("surface-gpu-id=%d,%s-gpu-id=%d",surface->gpu_id,GST_ELEMENT_NAME(object),\
         object->gpu_id)); \
    _errtype = 1;\
    } \
    } while(0); \
    _errtype; \
  })

/** Defines the types of buffer memory. */
typedef enum
{
  CUDA_PINNED_MEMORY,  /**< Pinned Host memory allocated using cudaMallocHost */
  CUDA_DEVICE_MEMORY,  /**< Device memory allocated using cudaMalloc */
  CUDA_UNIFIED_MEMORY  /**< Device memory allocated using cudaMallocManaged */
} NvBufMemType;

/** Holds the video frame buffer pointers and related information. */
typedef struct _NvBufSurface {
    /** Width of the frames contained in the buffer, in pixels. */
    unsigned int width;
    /** Height of the frames contained in the buffer, in pixels. */
    unsigned int height;
    /** Size of the frames contained in the buffer, in bytes. */
    unsigned int size;
    /** Size of the frames contained in the buffer, in bytes. */
    unsigned int num_batched_buffers;
    /** GPU ID used for allocation of memory */
    unsigned int gpu_id;
    /** Type of the buffer memory. */
    NvBufMemType mem_type;
    /** Pointer to the contiguous memory of batched buffers. */
    void *allocated_mem;
    /** Array of pointers to the individual buffers in the batched buffer. */
    void *buf_data[MAX_NVBUFFERS];
} NvBufSurface;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _NVBUFFER_H_ */
