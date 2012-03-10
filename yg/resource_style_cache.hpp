#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/set.hpp"
#include "../geometry/point2d.hpp"

namespace yg
{
  namespace gl
  {
    class PacketsQueue;
  }

  class StraightTextElement;
  class PathTextElement;
  class SkinPage;
  class GlyphCache;
  class ResourceManager;
  struct Color;
  struct PenInfo;
  struct CircleInfo;
  struct GlyphKey;

  class ResourceStyleCacheContext
  {
  private:

    struct Impl;
    Impl * m_impl;

    /// noncopyable
    ResourceStyleCacheContext(ResourceStyleCacheContext const & src);
    ResourceStyleCacheContext const & operator=(ResourceStyleCacheContext const & src);

  public:

    ResourceStyleCacheContext();
    ~ResourceStyleCacheContext();

    bool hasColor(Color const & c) const;
    void addColor(Color const & c);

    bool hasPenInfo(PenInfo const & pi) const;
    void addPenInfo(PenInfo const & pi);

    bool hasCircleInfo(CircleInfo const & ci) const;
    void addCircleInfo(CircleInfo const & ci);

    bool hasGlyph(GlyphKey const & gk) const;
    void addGlyph(GlyphKey const & gk);

    void clear();
  };

  /// - cache of potentially all yg::ResourceStyle's
  /// - cache is build on the separate thread (CoverageGenerator thread)
  /// - it is used to remove texture uploading code from the GUI-thread
  class ResourceStyleCache
  {
  private:

    shared_ptr<ResourceManager> m_rm;
    GlyphCache * m_glyphCache;
    yg::gl::PacketsQueue * m_glQueue;

    shared_ptr<SkinPage> m_cachePage;

  public:

    ResourceStyleCache(shared_ptr<ResourceManager> const & rm,
                        int glyphCacheID,
                        yg::gl::PacketsQueue * glQueue);

    ~ResourceStyleCache();

    shared_ptr<SkinPage> const & cachePage();

    shared_ptr<ResourceManager> const & resourceManager();

    GlyphCache * glyphCache();

    void upload();
    void clear();

    bool hasRoom(m2::PointU const * sizes, size_t cnt) const;
  };
}
