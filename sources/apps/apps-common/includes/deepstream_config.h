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

#ifndef __NVGSTDS_CONFIG_H__
#define __NVGSTDS_CONFIG_H__

#define HAS_NVGPU 1
#define HAS_MEMORY_FEATURES 1
#define MEMORY_FEATURES "memory:NVMM"

#define NVDS_ELEM_SRC_CAMERA_CSI "videotestsrc"
#define NVDS_ELEM_SRC_CAMERA_V4L2 "v4l2src"
#define NVDS_ELEM_SRC_URI "uridecodebin"

#define NVDS_ELEM_QUEUE "queue"
#define NVDS_ELEM_CAPS_FILTER "capsfilter"
#define NVDS_ELEM_TEE "tee"

#define NVDS_ELEM_PGIE "nvinfer"
#define NVDS_ELEM_SGIE "nvinfer"
#define NVDS_ELEM_TRACKER "nvtracker"

#define NVDS_ELEM_VIDEO_CONV "nvvidconv"
#define NVDS_ELEM_STREAM_MUX "nvstreammux"
#define NVDS_ELEM_STREAM_DEMUX "nvstreamdemux"
#define NVDS_ELEM_TILER "nvmultistreamtiler"
#define NVDS_ELEM_OSD "nvosd"
#define NVDS_ELEM_DSEXAMPLE_ELEMENT "dsexample"

#define NVDS_ELEM_DEWARPER "nvdewarper"
#define NVDS_ELEM_SPOTANALYSIS "nvspot"
#define NVDS_ELEM_NVAISLEMETADATA "nvaislemetadata"
#define NVDS_ELEM_BBOXFILTER "nvbboxfilter"
#define NVDS_ELEM_MSG_CONV "nvmsgconv"
#define NVDS_ELEM_MSG_BROKER "nvmsgbroker"

#define NVDS_ELEM_SINK_FAKESINK "fakesink"
#define NVDS_ELEM_SINK_FILE "filesink"
#define NVDS_ELEM_SINK_EGL "nveglglessink"

#define NVDS_ELEM_MUX_MP4 "qtmux"
#define NVDS_ELEM_MKV "matroskamux"

#define NVDS_ELEM_ENC_H264 "x264enc"
#define NVDS_ELEM_ENC_H265 "x265enc"
#define NVDS_ELEM_ENC_MPEG4 "avenc_mpeg4"

#define MAX_SOURCE_BINS 128
#define MAX_SINK_BINS (MAX_SOURCE_BINS * 4)
#define MAX_SECONDARY_GIE_BINS (16)

#define MAX_BBOXES 1000
#define MAX_BBOX_TEXT_SIZE 128

#endif
