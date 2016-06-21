#pragma once
#include "indexer/feature_loader_base.hpp"


namespace old_101 { namespace feature
{
  class LoaderImpl : public ::feature::LoaderBase
  {
    typedef ::feature::LoaderBase BaseT;

    /// Get the index for geometry serialization.
    /// @param[in]  scale:
    /// -1 : index for the best geometry
    /// default : needed geometry
    //@{
    int GetScaleIndex(int scale) const;
    int GetScaleIndex(int scale, offsets_t const & offsets) const;
    //@}

    enum
    {
      HEADER_HAS_LAYER = 1U << 7,
      HEADER_HAS_NAME = 1U << 6,
      HEADER_IS_AREA = 1U << 5,
      HEADER_IS_LINE = 1U << 4,
      HEADER_HAS_POINT = 1U << 3
    };

  public:
    LoaderImpl(::feature::SharedLoadInfo const & info) : BaseT(info) {}

    /// LoaderBase overrides:
    uint8_t GetHeader() override;
    void ParseTypes() override;
    void ParseCommon() override;
    void ParseHeader2() override;
    uint32_t ParseGeometry(int scale) override;
    uint32_t ParseTriangles(int scale) override;
    void ParseMetadata() override {} /// not supported in this version
    void ParseAltitude() override {} /// not supported in this version
  };
}
}
