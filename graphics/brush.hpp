#pragma once

#include "graphics/resource.hpp"
#include "graphics/color.hpp"

namespace graphics
{
  struct Brush : public Resource
  {
    struct Info : public Resource::Info
    {
      Color m_color;

      Info();
      explicit Info(Color const & color);

      Resource::Info const & cacheKey() const;
      m2::PointU const resourceSize() const;
      Resource * createResource(m2::RectU const & texRect,
                                uint8_t pipelineID) const;

      bool lessThan(Resource::Info const * r) const;
    };

    Info m_info;

    Brush(m2::RectU const & texRect,
          uint8_t pipelineID,
          Info const & info);

    void render(void * dst);
    Resource::Info const * info() const;
  };
}
