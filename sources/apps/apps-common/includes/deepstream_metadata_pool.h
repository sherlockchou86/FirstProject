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

#ifndef __NVGSTDS_META_POOL_H__
#define __NVGSTDS_META_POOL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>
#include "gstnvdsmeta.h"

typedef struct
{
  GQueue pool_queue;
  guint pool_size;
  GMutex pool_lock;
  GCond pool_cond;
} NvDsMetaPool;

/**
 * Initialize the pool and allocate required memory for @ref NvDsFrameMeta.
 * Allocated pool should be freed using ::nvds_meta_pool_clear
 *
 * @param[in] pool pointer to @ref NvDsMetaPool
 * @param[in] pool_size number of objects of type @ref NvDsFrameMeta
 *            to allocate.
 */
void nvds_meta_pool_init (NvDsMetaPool *pool, guint pool_size);

/**
 * Free the memory and other resources of pool.
 *
 * @param[in] pool pointer to pool to be freed.
 */
void nvds_meta_pool_clear (NvDsMetaPool *pool);

/**
 * Retrieve next available @ref NvDsFrameMeta object from pool.
 *
 * @param[in] pool pointer to pool
 *
 * @return returns the pointer to available @ref NvDsFrameMeta object.
 */
NvDsFrameMeta *nvds_meta_pool_get_free_meta (NvDsMetaPool *pool);

/**
 * Release the meta data object back to pool.
 *
 * @param[in] meta pointer to the meta data to be returned to pool.s
 */
void nvds_meta_pool_release_meta_to_pool (gpointer meta);

#ifdef __cplusplus
}
#endif

#endif
