#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/vector.hpp"

namespace yg
{
  class StraightTextElement;
  class PathTextElement;
  class SkinPage;
  class GlyphCache;
  class ResourceManager;

  class StylesCache
  {
  private:

    vector<shared_ptr<SkinPage> > m_cachePages;
    shared_ptr<ResourceManager> m_rm;
    GlyphCache * m_glyphCache;
    int m_maxPagesCount;

  public:

    StylesCache(shared_ptr<ResourceManager> const & rm,
                int glyphCacheID,
                int maxPagesCount);

    ~StylesCache();

    vector<shared_ptr<SkinPage> > & cachePages();

    shared_ptr<ResourceManager> const & resourceManager();

    GlyphCache * glyphCache();

    void upload();
  };
}
