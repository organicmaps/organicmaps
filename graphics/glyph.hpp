#pragma once

#include "graphics/resource.hpp"
#include "graphics/glyph_cache.hpp"

namespace graphics
{
  struct Glyph : public Resource
  {
    struct Info : public Resource::Info
    {
      GlyphKey m_key;
      GlyphMetrics m_metrics;
      GlyphCache * m_cache;

      Info();
      Info(GlyphKey const & key,
           GlyphCache * cache);

      Resource::Info const & cacheKey() const;
      m2::PointU const resourceSize() const;
      Resource * createResource(m2::RectU const & texRect,
                                uint8_t pipelineID) const;

      bool lessThan(Resource::Info const * r) const;
    };

    Info m_info;

    shared_ptr<GlyphBitmap> m_bitmap;

    Glyph(Info const & info,
          m2::RectU const & texRect,
          int pipelineID);

    void render(void * dst);
    Resource::Info const * info() const;
  };
}
