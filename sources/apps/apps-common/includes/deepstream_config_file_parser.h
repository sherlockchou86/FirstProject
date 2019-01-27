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

#ifndef __NVGSTDS_CONFIG_PARSER_H__
#define  __NVGSTDS_CONFIG_PARSER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <gst/gst.h>
#include "deepstream_config.h"
#include "deepstream_sources.h"
#include "deepstream_primary_gie.h"
#include "deepstream_tiled_display.h"
#include "deepstream_gie.h"
#include "deepstream_sinks.h"
#include "deepstream_osd.h"
#include "deepstream_sources.h"
#include "deepstream_dsexample.h"
#include "deepstream_streammux.h"
#include "deepstream_tracker.h"
#include "deepstream_dewarper.h"

#define CONFIG_GROUP_SOURCE "source"
#define CONFIG_GROUP_OSD "osd"
#define CONFIG_GROUP_PRIMARY_GIE "primary-gie"
#define CONFIG_GROUP_SECONDARY_GIE "secondary-gie"
#define CONFIG_GROUP_TRACKER "tracker"
#define CONFIG_GROUP_SINK "sink"
#define CONFIG_GROUP_TILED_DISPLAY "tiled-display"
#define CONFIG_GROUP_DSEXAMPLE "ds-example"
#define CONFIG_GROUP_STREAMMUX "streammux"
#define CONFIG_GROUP_DEWARPER "dewarper"


/**
 * Function to read properties of source element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsDewarperConfig
 * @param[in] key_file pointer to file having key value pairs.
 * @param[in] group name of property group @ref CONFIG_GROUP_DEWARPER
 * @param[in] cfg_file_path path of configuration file.
 *
 * @return true if parsed successfully.
 */
gboolean
parse_dewarper (NvDsDewarperConfig * config, GKeyFile * key_file, gchar *cfg_file_path);

/**
 * Function to read properties of source element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsSourceConfig
 * @param[in] key_file pointer to file having key value pairs.
 * @param[in] group name of property group @ref CONFIG_GROUP_SOURCE
 * @param[in] cfg_file_path path of configuration file.
 *
 * @return true if parsed successfully.
 */
gboolean
parse_source (NvDsSourceConfig * config, GKeyFile * key_file,
    gchar * group, gchar * cfg_file_path);

/**
 * Function to read properties of OSD element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsOSDConfig
 * @param[in] key_file pointer to file having key value pairs.
 *
 * @return true if parsed successfully.
 */
gboolean parse_osd (NvDsOSDConfig * config, GKeyFile * key_file);

/**
 * Function to read properties of infer element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsGieConfig
 * @param[in] key_file pointer to file having key value pairs.
 * @param[in] group name of property group @ref CONFIG_GROUP_PRIMARY_GIE and
 *            @ref CONFIG_GROUP_SECONDARY_GIE
 * @param[in] cfg_file_path path of configuration file.
 *
 * @return true if parsed successfully.
 */
gboolean
parse_gie (NvDsGieConfig * config, GKeyFile * key_file, gchar * group,
    gchar * cfg_file_path);

/**
 * Function to read properties of tracker element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsTrackerConfig
 * @param[in] key_file pointer to file having key value pairs.
 *
 * @return true if parsed successfully.
 */
gboolean
parse_tracker (NvDsTrackerConfig * config, GKeyFile * key_file);

/**
 * Function to read properties of sink element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsSinkSubBinConfig
 * @param[in] key_file pointer to file having key value pairs.
 * @param[in] group name of property group @ref CONFIG_GROUP_SINK
 *
 * @return true if parsed successfully.
 */
gboolean
parse_sink (NvDsSinkSubBinConfig * config, GKeyFile * key_file, gchar * group);

/**
 * Function to read properties of tiler element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsTiledDisplayConfig
 * @param[in] key_file pointer to file having key value pairs.
 *
 * @return true if parsed successfully.
 */
gboolean
parse_tiled_display (NvDsTiledDisplayConfig * config, GKeyFile * key_file);

/**
 * Function to read properties of dsexample element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsDsExampleConfig
 * @param[in] key_file pointer to file having key value pairs.
 *
 * @return true if parsed successfully.
 */
gboolean
parse_dsexample (NvDsDsExampleConfig * config, GKeyFile * key_file);

/**
 * Function to read properties of streammux element from configuration file.
 *
 * @param[in] config pointer to @ref NvDsStreammuxConfig
 * @param[in] key_file pointer to file having key value pairs.
 *
 * @return true if parsed successfully.
 */
gboolean
parse_streammux (NvDsStreammuxConfig * config, GKeyFile * key_file);

/**
 * Utility function to convert relative path in configuration file
 * with absolute path.
 *
 * @param[in] cfg_file_path path of configuration file.
 * @param[in] file_path relative path of file.
 */
gchar *
get_absolute_file_path (gchar *cfg_file_path, gchar * file_path);


#ifdef __cplusplus
}
#endif

#endif
