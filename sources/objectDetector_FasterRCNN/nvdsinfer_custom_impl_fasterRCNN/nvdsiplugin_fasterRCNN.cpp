/**
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#include "nvdsinfer_custom_impl.h"
#include "factoryFasterRCNNLegacy.h"
#include "factoryFasterRCNN.h"

// Uncomment to use the legacy IPluginFactory interface
//#define USE_LEGACY_IPLUGIN_FACTORY

bool NvDsInferPluginFactoryCaffeGet (NvDsInferPluginFactoryCaffe &pluginFactory,
    NvDsInferPluginFactoryType &type)
{
#ifdef USE_LEGACY_IPLUGIN_FACTORY
  type = PLUGIN_FACTORY;
  pluginFactory.pluginFactory = new FRCNNPluginFactoryLegacy;
#else
  type = PLUGIN_FACTORY_V2;
  pluginFactory.pluginFactoryV2 = new FRCNNPluginFactory;
#endif

  return true;
}

void NvDsInferPluginFactoryCaffeDestroy (NvDsInferPluginFactoryCaffe &pluginFactory)
{
#ifdef USE_LEGACY_IPLUGIN_FACTORY
  FRCNNPluginFactoryLegacy *factory =
      static_cast<FRCNNPluginFactoryLegacy *> (pluginFactory.pluginFactory);
#else
  FRCNNPluginFactory *factory =
      static_cast<FRCNNPluginFactory *> (pluginFactory.pluginFactoryV2);
#endif
  factory->destroyPlugin();
  delete factory;
}

#ifdef USE_LEGACY_IPLUGIN_FACTORY
bool NvDsInferPluginFactoryRuntimeGet (nvinfer1::IPluginFactory *& pluginFactory)
{
  pluginFactory = new FRCNNPluginFactoryLegacy;
  return true;
}

void NvDsInferPluginFactoryRuntimeDestroy (nvinfer1::IPluginFactory * pluginFactory)
{
  FRCNNPluginFactoryLegacy *factory =
      static_cast<FRCNNPluginFactoryLegacy *> (pluginFactory);
  factory->destroyPlugin();
  delete factory;
}
#endif
