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

#include "dsexample_lib.h"
#include <stdio.h>
#include <stdlib.h>

struct DsExampleCtx
{
    DsExampleInitParams initParams;
};

DsExampleCtx *
DsExampleCtxInit (DsExampleInitParams * initParams)
{
    DsExampleCtx *ctx = (DsExampleCtx *) calloc (1, sizeof (DsExampleCtx));
    ctx->initParams = *initParams;
    return ctx;
}

// In case of an actual processing library, processing on data wil be completed
// in this function and output will be returned
DsExampleOutput *
DsExampleProcess (DsExampleCtx * ctx, unsigned char *data)
{
    DsExampleOutput *out =
        (DsExampleOutput*)calloc (1, sizeof (DsExampleOutput));

    if (data != NULL)
    {
        // Process your data here
    }
    // Fill output structure using processed output
    // Here, we fake some detected objects and labels
    if (ctx->initParams.fullFrame)
    {
        out->numObjects = 2;
        out->object[0] = (DsExampleObject)
        {
            ctx->initParams.processingWidth/8,
                ctx->initParams.processingHeight/8,
                ctx->initParams.processingWidth/8,
                ctx->initParams.processingHeight/8, "Obj0"
        };

        out->object[1] = (DsExampleObject)
        {
            ctx->initParams.processingWidth/2,
                ctx->initParams.processingHeight/2,
                ctx->initParams.processingWidth/8,
                ctx->initParams.processingHeight/8, "Obj1"
        };
    }
    else
    {
        out->numObjects = 1;
        out->object[0] = (DsExampleObject)
        {
            ctx->initParams.processingWidth/8,
                ctx->initParams.processingHeight/8,
                ctx->initParams.processingWidth/8,
                ctx->initParams.processingHeight/8, ""
        };
        // Set the object label
        snprintf (out->object[0].label, 64, "Obj_label");
    }

    return out;
}

void
DsExampleCtxDeinit (DsExampleCtx * ctx)
{
    free (ctx);
}

