#pragma once

#include "std/string.hpp"

#include "graphics/resource.hpp"

namespace graphics
{
  struct Icon : public Resource
  {
    struct Info : public Resource::Info
    {
      string m_name;

      Info();
      Info(string const & name);

      Resource::Info const & cacheKey() const;
      m2::PointU const resourceSize() const;
      Resource * createResource(m2::RectU const & texRect,
                                uint8_t pipelineID) const;
      bool lessThan(Resource::Info const * r) const;
    };

    Info m_info;

    Icon(m2::RectU const & texRect,
         int pipelineID,
         Info const & info);

    void render(void * dst);
    Resource::Info const * info() const;
  };
}
