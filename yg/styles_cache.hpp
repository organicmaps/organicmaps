#pragma once

#include "../std/shared_ptr.hpp"
#include "../geometry/point2d.hpp"

namespace yg
{
  class StraightTextElement;
  class PathTextElement;
  class SkinPage;
  class GlyphCache;
  class ResourceManager;

  /// - cache of potentially all yg::ResourceStyle's
  /// - cache is build on the separate thread (CoverageGenerator thread)
  /// - it is used to remove texture uploading code from the GUI-thread
  class StylesCache
  {
  private:

    shared_ptr<ResourceManager> m_rm;
    GlyphCache * m_glyphCache;

    shared_ptr<SkinPage> m_cachePage;

  public:

    StylesCache(shared_ptr<ResourceManager> const & rm,
                int glyphCacheID);

    ~StylesCache();

    shared_ptr<SkinPage> const & cachePage();

    shared_ptr<ResourceManager> const & resourceManager();

    GlyphCache * glyphCache();

    void upload();
    void clear();

    bool hasRoom(m2::PointU const * sizes, size_t cnt) const;
  };
}
