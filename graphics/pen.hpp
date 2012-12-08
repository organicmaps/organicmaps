#pragma once

#include "resource.hpp"
#include "color.hpp"

#include "../geometry/point2d.hpp"

#include "../base/buffer_vector.hpp"

namespace graphics
{
  struct Pen : public Resource
  {
    /// definition of the line style pattern
    /// used as a texture-cache-key
    struct Info : public Resource::Info
    {
      typedef buffer_vector<double, 16> TPattern;
      Color m_color;
      double m_w;
      TPattern m_pat;
      double m_offset;

      bool m_isSolid;

      Info();
      Info(Color const & color,
           double width,
           double const * pattern,
           size_t patternSize,
           double offset);

      double firstDashOffset() const;
      bool atDashOffset(double offset) const;

      m2::PointU const resourceSize() const;
      Resource * createResource(m2::RectU const & texRect,
                                uint8_t pipelineID) const;

      bool lessThan(Resource::Info const * r) const;
    };

    Info m_info;

    bool m_isWrapped;
    bool m_isSolid;

    m2::PointU m_centerColorPixel;
    m2::PointU m_borderColorPixel;

    Pen(bool isWrapped,
        m2::RectU const & texRect,
        int pipelineID,
        Info const & info);

    /// with antialiasing zones
    double geometryTileLen() const;
    double geometryTileWidth() const;

    /// without antialiasing zones
    double rawTileLen() const;
    double rawTileWidth() const;

    void render(void * dst);
    Resource::Info const * info() const;
  };
}
