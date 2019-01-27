/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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
 * <b>NVIDIA DeepStream: Message Schema Generation Library Interface</b>
 *
 * @b Description: This file specifies the NVIDIA DeepStream message schema generation
 * library interface.
 */

#ifndef NVMSGCONV_H_
#define NVMSGCONV_H_


#include "nvdsmeta.h"
#include <glib.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @ref NvDsMsg2pCtx is structure for library context.
 */
typedef struct NvDsMsg2pCtx {
  /** type of payload to be generated. */
  NvDsPayloadType payloadType;

  /** private to component. Don't change this field. */
  gpointer privData;
} NvDsMsg2pCtx;

/**
 * This function initializes the library with user defined options mentioned
 * in the file and returns the handle to the context.
 * Static fields which should be part of message payload can be added to
 * file instead of frame metadata.
 *
 * @param[in] file name of file to read static properties from.
 * @param[in] type type of payload to be generated.
 *
 * @return pointer to library context created. This context should be used in
 * other functions of library and should be freed with
 * @ref nvds_msg2p_ctx_destroy
 */
NvDsMsg2pCtx* nvds_msg2p_ctx_create (const gchar *file, NvDsPayloadType type);

/**
 * Release the resources allocated during context creation.
 *
 * @param[in] ctx pointer to library context.
 */
void nvds_msg2p_ctx_destroy (NvDsMsg2pCtx *ctx);

/**
 * This function will parse the @ref NvDsEventMsgMeta and will generate message
 * payload. Payload will be combination of static values read from
 * configuration file and dynamic values received in meta.
 * Payload will be generated based on the @ref NvDsPayloadType type provided
 * in context creation (e.g. Deepstream, Custom etc.).
 *
 * @param[in] ctx pointer to library context.
 * @param[in] events pointer to array of event objects.
 * @param[in] size number of objects in array.
 *
 * @return pointer to @ref NvDsPayload generated or NULL in case of error.
 * This payload should be freed with @ref nvds_msg2p_release
 */
NvDsPayload*
nvds_msg2p_generate (NvDsMsg2pCtx *ctx, NvDsEvent *events, guint size);

/**
 * This function should be called to release memory allocated for payload.
 *
 * @param[in] ctx pointer to library context.
 * @param[in] payload pointer to object that needs to be released.
 */
void nvds_msg2p_release (NvDsMsg2pCtx *ctx, NvDsPayload *payload);

#ifdef __cplusplus
}
#endif
#endif /* NVMSGCONV_H_ */
