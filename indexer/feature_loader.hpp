#pragma once
#include "feature_loader_base.hpp"


namespace feature
{
  class LoaderCurrent : public LoaderBase
  {
    typedef LoaderBase BaseT;

    /// Get the index for geometry serialization.
    /// @param[in]  scale:
    /// -1 : index for the best geometry
    /// -2 : index for the worst geometry
    /// default : needed geometry
    //@{
    int GetScaleIndex(int scale) const;
    int GetScaleIndex(int scale, offsets_t const & offsets) const;
    //@}

  public:
    LoaderCurrent(SharedLoadInfo const & info) : BaseT(info) {}

    virtual uint8_t GetHeader();

    virtual void ParseTypes();
    virtual void ParseCommon();
    virtual void ParseHeader2();
    virtual uint32_t ParseGeometry(int scale);
    virtual uint32_t ParseTriangles(int scale);
    virtual void ParseMetadata();
  };
}
