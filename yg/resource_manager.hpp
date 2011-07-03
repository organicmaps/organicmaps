#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"
#include "../std/string.hpp"
#include "../std/list.hpp"

#include "../base/mutex.hpp"
#include "../base/threaded_list.hpp"

#include "storage.hpp"
#include "glyph_cache.hpp"

namespace yg
{
  class Skin;

  namespace gl
  {
    class BaseTexture;
    class Storage;
  }

  struct GlyphInfo;

  enum RtFormat
  {
    Rt8Bpp,
    Rt4Bpp
  };

  class ResourceManager
  {
  private:

    typedef map<string, shared_ptr<gl::BaseTexture> > TStaticTextures;

    TStaticTextures m_staticTextures;

    threads::Mutex m_mutex;

    size_t m_dynamicTextureWidth;
    size_t m_dynamicTextureHeight;

    ThreadedList<shared_ptr<gl::BaseTexture> > m_dynamicTextures;

    size_t m_fontTextureWidth;
    size_t m_fontTextureHeight;

    ThreadedList<shared_ptr<gl::BaseTexture> > m_fontTextures;

    size_t m_tileTextureWidth;
    size_t m_tileTextureHeight;

    ThreadedList<shared_ptr<gl::BaseTexture> > m_renderTargets;

    size_t m_vbSize;
    size_t m_ibSize;

    size_t m_smallVBSize;
    size_t m_smallIBSize;

    size_t m_blitVBSize;
    size_t m_blitIBSize;

    ThreadedList<gl::Storage> m_storages;
    ThreadedList<gl::Storage> m_smallStorages;
    ThreadedList<gl::Storage> m_blitStorages;

    vector<GlyphCache> m_glyphCaches;

    RtFormat m_format;

    bool m_useVA;
    bool m_fillSkinAlpha;

    size_t m_storagesCount;
    size_t m_smallStoragesCount;
    size_t m_blitStoragesCount;
    size_t m_dynamicTexturesCount;
    size_t m_fontTexturesCount;

  public:

    ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                    size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                    size_t blitVBSize, size_t blitIBSize, size_t blitStoragesCount,
                    size_t texWidth, size_t texHeight, size_t texCount,
                    size_t fontTexWidth, size_t fontTexHeight, size_t fontTexCount,
                    size_t tileTexWidth, size_t tileTexHeight, size_t tileTexCount,
                    char const * blocksFile, char const * whileListFile, char const * blackListFile,
                    size_t glyphCacheSize,
                    RtFormat fmt,
                    bool useVA,
                    bool fillSkinAlpha);

    shared_ptr<gl::BaseTexture> const & getTexture(string const & fileName);

    ThreadedList<gl::Storage> & storages();
    ThreadedList<gl::Storage> & smallStorages();
    ThreadedList<gl::Storage> & blitStorages();
    ThreadedList<shared_ptr<gl::BaseTexture> > & dynamicTextures();
    ThreadedList<shared_ptr<gl::BaseTexture> > & fontTextures();
    ThreadedList<shared_ptr<gl::BaseTexture> > & renderTargets();

    size_t dynamicTextureWidth() const;
    size_t dynamicTextureHeight() const;

    size_t fontTextureWidth() const;
    size_t fontTextureHeight() const;

    size_t tileTextureWidth() const;
    size_t tileTextureHeight() const;

    shared_ptr<GlyphInfo> const getGlyphInfo(GlyphKey const & key);
    GlyphMetrics const getGlyphMetrics(GlyphKey const & key);
    GlyphCache * glyphCache(int glyphCacheID = 0);

    int renderThreadGlyphCacheID(int threadNum) const;
    int guiThreadGlyphCacheID() const;

    void addFonts(vector<string> const & fontNames);

    void memoryWarning();
    void enterBackground();
    void enterForeground();

    shared_ptr<yg::gl::BaseTexture> createRenderTarget(unsigned w, unsigned h);

    bool fillSkinAlpha() const;
  };

  Skin * loadSkin(shared_ptr<ResourceManager> const & resourceManager,
                  string const & fileName,
                  size_t dynamicPagesCount,
                  size_t textPagesCount);
}

