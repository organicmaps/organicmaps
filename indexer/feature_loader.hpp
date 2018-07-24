#pragma once
#include "indexer/feature_loader_base.hpp"

#include "indexer/feature.hpp"

namespace feature
{
  class LoaderCurrent : public LoaderBase
  {
    /// Get the index for geometry serialization.
    /// @param[in]  scale:
    /// -1 : index for the best geometry
    /// -2 : index for the worst geometry
    /// default : needed geometry
    //@{
    int GetScaleIndex(int scale) const;
    int GetScaleIndex(int scale, FeatureType::GeometryOffsets const & offsets) const;
    //@}

  public:
    LoaderCurrent(SharedLoadInfo const & info) : LoaderBase(info) {}
    /// LoaderBase overrides:
    uint8_t GetHeader(FeatureType const & ft) const override;
    void ParseTypes(FeatureType & ft) const override;
    void ParseCommon(FeatureType & ft) const override;
    void ParseHeader2(FeatureType & ft) const override;
    uint32_t ParseGeometry(int scale, FeatureType & ft) const override;
    uint32_t ParseTriangles(int scale, FeatureType & ft) const override;
    void ParseMetadata(FeatureType & ft) const override;
  };
}
