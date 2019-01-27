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
#include "nvdssample_fasterRCNN_common.h"

#include <cassert>
#include <cstring>
#include <memory>

using namespace nvinfer1;
using namespace nvcaffeparser1;
using namespace plugin;

class FRCNNPluginFactory : public nvcaffeparser1::IPluginFactoryV2
{
public:
  virtual nvinfer1::IPluginV2* createPlugin(const char* layerName,
      const nvinfer1::Weights* weights, int nbWeights,
      const char* libNamespace) override
  {
    assert(isPluginV2(layerName));
    if (!strcmp(layerName, "RPROIFused"))
    {
      assert(mPluginRPROI == nullptr);
      assert(nbWeights == 0 && weights == nullptr);
      mPluginRPROI = std::unique_ptr<IPluginV2, decltype(pluginDeleter)>(
          createRPNROIPlugin(featureStride, preNmsTop, nmsMaxOut, iouThreshold,
              minBoxSize, spatialScale, DimsHW(poolingH, poolingW),
              Weights{nvinfer1::DataType::kFLOAT, anchorsRatios, anchorsRatioCount},
              Weights{nvinfer1::DataType::kFLOAT, anchorsScales, anchorsScaleCount}),
          pluginDeleter);
      mPluginRPROI.get()->setPluginNamespace(libNamespace);
      return mPluginRPROI.get();
    }
    else
    {
      assert(0);
      return nullptr;
    }
  }

  // caffe parser plugin implementation
  bool isPluginV2(const char* name) override { return !strcmp(name, "RPROIFused"); }

  void destroyPlugin()
  {
    mPluginRPROI.reset();
  }

  void (*pluginDeleter)(IPluginV2*){[](IPluginV2* ptr) { ptr->destroy(); }};
  std::unique_ptr<IPluginV2, decltype(pluginDeleter)> mPluginRPROI{nullptr, pluginDeleter};

  virtual ~FRCNNPluginFactory()
  {
  }
};
