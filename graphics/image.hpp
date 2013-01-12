#pragma once

#include "resource.hpp"

#include "../std/string.hpp"

#include "../geometry/point2d.hpp"

namespace graphics
{
  /// get dimensions of PNG image specified by it's resourceName.
  m2::PointU const GetDimensions(string const & resourceName);

  struct Image : public Resource
  {
    struct Info : public Resource::Info
    {
      string m_resourceName;
      m2::PointU m_size;
      vector<unsigned char> m_data;

      Info();
      Info(char const * resName);

      unsigned width() const;
      unsigned height() const;
      unsigned char const * data() const;

      Resource::Info const & cacheKey() const;
      m2::PointU const resourceSize() const;
      Resource * createResource(m2::RectU const & texRect,
                                uint8_t pipelineID) const;
      bool lessThan(Resource::Info const * r) const;
    };

    Info m_info;

    Image(m2::RectU const & texRect,
          int pipelineID,
          Info const & info);

    void render(void * dst);
    Resource::Info const * info() const;
  };
}
