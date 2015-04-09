#pragma once

#include "graphics/color.hpp"
#include "graphics/resource.hpp"

#include "geometry/point2d.hpp"

namespace graphics
{
  struct Circle : public Resource
  {
    struct Info : public Resource::Info
    {
      unsigned m_radius;
      Color m_color;
      bool m_isOutlined;
      unsigned m_outlineWidth;
      Color m_outlineColor;

      Info();
      Info(double radius,
           Color const & color = Color(0, 0, 0, 255),
           bool isOutlined = false,
           double outlineWidth = 1,
           Color const & outlineColor = Color(255, 255, 255, 255));

      Resource::Info const & cacheKey() const;
      m2::PointU const resourceSize() const;
      Resource * createResource(m2::RectU const & texRect,
                                uint8_t pipelineID) const;

      bool lessThan(Resource::Info const * r) const;
    };

    Info m_info;

    Circle(m2::RectU const & texRect,
           int pipelineID,
           Info const & info);

    void render(void * dst);
    Resource::Info const * info() const;
  };
}
