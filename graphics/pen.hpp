#pragma once

#include "graphics/resource.hpp"
#include "graphics/icon.hpp"
#include "graphics/color.hpp"

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"

namespace graphics
{
  struct Pen : public Resource
  {
    /// definition of the line style pattern
    /// used as a texture-cache-key
    struct Info : public Resource::Info
    {
      enum ELineJoin
      {
        ERoundJoin,
        EBevelJoin,
        ENoJoin
      };

      enum ELineCap
      {
        ERoundCap,
        EButtCap,
        ESquareCap
      };

      typedef buffer_vector<double, 16> TPattern;
      Color m_color;
      double m_w;
      TPattern m_pat;
      double m_offset;
      Icon::Info m_icon;
      double m_step;
      ELineJoin m_join;
      ELineCap m_cap;

      bool m_isSolid;

      Info(Color const & color = Color(0, 0, 0, 255),
           double width = 1.0,
           double const * pattern = 0,
           size_t patternSize = 0,
           double offset = 0,
           char const * symbol = 0,
           double step = 0,
           ELineJoin join = ERoundJoin,
           ELineCap cap = ERoundCap);

      double firstDashOffset() const;
      bool atDashOffset(double offset) const;

      Resource::Info const & cacheKey() const;
      m2::PointU const resourceSize() const;
      Resource * createResource(m2::RectU const & texRect,
                                uint8_t pipelineID) const;

      bool lessThan(Resource::Info const * r) const;
    };

    Info m_info;

    bool m_isSolid;

    m2::PointU m_centerColorPixel;
    m2::PointU m_borderColorPixel;

    Pen(m2::RectU const & texRect,
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
