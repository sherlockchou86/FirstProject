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

#include "NvCaffeParser.h"
#include "NvInferPlugin.h"
#include <memory>
#include <cassert>
#include <cstring>
#include "nvdssample_fasterRCNN_common.h"

using namespace nvinfer1;
using namespace nvcaffeparser1;
using namespace plugin;

// integration for serialization
class FRCNNPluginFactoryLegacy : public nvinfer1::IPluginFactory, public nvcaffeparser1::IPluginFactory
{
public:
  // deserialization plugin implementation
  virtual nvinfer1::IPlugin* createPlugin(const char* layerName,
      const nvinfer1::Weights* weights, int nbWeights) override
  {
    assert(isPlugin(layerName));
    if (!strcmp(layerName, "RPROIFused"))
    {
      assert(mPluginRPROI == nullptr);
      assert(nbWeights == 0 && weights == nullptr);
      mPluginRPROI = std::unique_ptr<INvPlugin, decltype(nvPluginDeleter)>(
          createFasterRCNNPlugin(featureStride, preNmsTop, nmsMaxOut,
              iouThreshold, minBoxSize, spatialScale, DimsHW(poolingH, poolingW),
              Weights{ nvinfer1::DataType::kFLOAT, anchorsRatios, anchorsRatioCount },
              Weights{ nvinfer1::DataType::kFLOAT, anchorsScales, anchorsScaleCount }),
          nvPluginDeleter);
      return mPluginRPROI.get();
    }
    else
    {
      assert(0);
      return nullptr;
    }
  }

  IPlugin* createPlugin(const char* layerName, const void* serialData,
      size_t serialLength) override
  {
    assert(isPlugin(layerName));
    if (!strcmp(layerName, "RPROIFused"))
    {
      assert(mPluginRPROI == nullptr);
      mPluginRPROI = std::unique_ptr<INvPlugin, decltype(nvPluginDeleter)>(
          createFasterRCNNPlugin(serialData, serialLength), nvPluginDeleter);
      return mPluginRPROI.get();
    }
    else
    {
      assert(0);
      return nullptr;
    }
  }

  // caffe parser plugin implementation
  bool isPlugin(const char* name) override { return !strcmp(name, "RPROIFused"); }

  // User application destroys plugin when it is safe to do so.
  // Should be done after consumers of plugin (like ICudaEngine) are destroyed.
  void destroyPlugin()
  {
    mPluginRPROI.reset();
  }

  void(*nvPluginDeleter)(INvPlugin*) { [](INvPlugin* ptr) {ptr->destroy(); } };
  std::unique_ptr<INvPlugin, decltype(nvPluginDeleter)> mPluginRPROI{ nullptr, nvPluginDeleter };

  virtual ~FRCNNPluginFactoryLegacy()
  {
  }
};
