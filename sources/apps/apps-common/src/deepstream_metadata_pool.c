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

#include "deepstream_metadata_pool.h"
#include "deepstream_config.h"
#include <string.h>

void
nvds_meta_pool_init (NvDsMetaPool *pool, guint pool_size)
{
  guint i, j;

  pool->pool_size = pool_size;
  g_queue_init (&pool->pool_queue);
  g_mutex_init (&pool->pool_lock);
  g_cond_init (&pool->pool_cond);
  for (i = 0; i < pool_size; i++) {
    gpointer data = g_malloc0 (sizeof (NvDsFrameMeta) + sizeof (NvDsMetaPool *));

    NvDsFrameMeta *params = data;
    params->obj_params = g_malloc0 (MAX_BBOXES * sizeof (NvDsObjectParams));
    for (j = 0; j < MAX_BBOXES; j++) {
      params->obj_params[j].text_params.display_text =
          g_malloc0 (MAX_BBOX_TEXT_SIZE);
    }

    *((NvDsMetaPool **) (params + 1)) = pool;
    g_queue_push_tail (&pool->pool_queue, params);
  }
}

void
nvds_meta_pool_clear (NvDsMetaPool *pool)
{
  NvDsFrameMeta *params;

  while ((params = (NvDsFrameMeta *) g_queue_pop_head (&pool->pool_queue))) {
    guint j;
    for (j = 0; j < MAX_BBOXES; j++) {
      g_free (params->obj_params[j].text_params.display_text);
    }
    g_free (params->obj_params);
    g_free (params);
  }

  g_queue_clear (&pool->pool_queue);
  g_mutex_clear (&pool->pool_lock);
  g_cond_clear (&pool->pool_cond);
}

NvDsFrameMeta *
nvds_meta_pool_get_free_meta (NvDsMetaPool *pool)
{
  NvDsFrameMeta *params = NULL;

  g_mutex_lock (&pool->pool_lock);
  while (!(params = g_queue_pop_head (&pool->pool_queue))) {
    g_cond_wait (&pool->pool_cond, &pool->pool_lock);
  }
  g_mutex_unlock (&pool->pool_lock);

  if (params) {
    params->nvosd_mode = 0;
    params->num_rects = 0;
    params->num_strings = 0;
  }

  return params;
}

void
nvds_meta_pool_release_meta_to_pool (gpointer meta)
{
  NvDsFrameMeta *params = (NvDsFrameMeta *) meta;
  NvDsMetaPool *pool = *((NvDsMetaPool **) (params + 1));

  g_mutex_lock (&pool->pool_lock);
  g_queue_push_tail (&pool->pool_queue, params);
  g_cond_broadcast (&pool->pool_cond);
  g_mutex_unlock (&pool->pool_lock);
}
