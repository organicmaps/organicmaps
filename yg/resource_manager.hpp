#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"
#include "../std/string.hpp"
#include "../std/list.hpp"

#include "../base/mutex.hpp"

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

    list<shared_ptr<gl::BaseTexture> > m_dynamicTextures;

    size_t m_fontTextureWidth;
    size_t m_fontTextureHeight;

    list<shared_ptr<gl::BaseTexture> > m_fontTextures;

    size_t m_vbSize;
    size_t m_ibSize;

    size_t m_smallVBSize;
    size_t m_smallIBSize;

    size_t m_blitVBSize;
    size_t m_blitIBSize;

    list<gl::Storage> m_storages;
    list<gl::Storage> m_smallStorages;
    list<gl::Storage> m_blitStorages;

    gl::Storage const reserveStorageImpl(bool doWait, list<gl::Storage> & l);
    void freeStorageImpl(gl::Storage const & storage, bool doSignal, list<gl::Storage> & l);

    shared_ptr<gl::BaseTexture> const reserveTextureImpl(bool doWait, list<shared_ptr<gl::BaseTexture> > & l);
    void freeTextureImpl(shared_ptr<gl::BaseTexture> const & texture, bool doSignal, list<shared_ptr<gl::BaseTexture> > & l);

    vector<GlyphCache> m_glyphCaches;

    RtFormat m_format;

    bool m_useVA;
    bool m_fillSkinAlpha;

  public:

    ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                    size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                    size_t blitVBSize, size_t blitIBSize, size_t blitStoragesCount,
                    size_t texWidth, size_t texHeight, size_t texCount,
                    size_t fontTexWidth, size_t fontTexHeight, size_t fontTexCount,
                    char const * blocksFile, char const * whileListFile, char const * blackListFile,
                    size_t primaryGlyphCacheSize,
                    size_t secondaryGlyphCacheSize,
                    RtFormat fmt,
                    bool useVA,
                    bool fillSkinAlpha);

    shared_ptr<gl::BaseTexture> const & getTexture(string const & fileName);

    gl::Storage const reserveStorage(bool doWait = false);
    void freeStorage(gl::Storage const & storage, bool doSignal = false);

    gl::Storage const reserveSmallStorage(bool doWait = false);
    void freeSmallStorage(gl::Storage const & storage, bool doSignal = false);

    gl::Storage const reserveBlitStorage(bool doWait = false);
    void freeBlitStorage(gl::Storage const & storage, bool doSignal = false);

    shared_ptr<gl::BaseTexture> const reserveDynamicTexture(bool doWait = false);
    void freeDynamicTexture(shared_ptr<gl::BaseTexture> const & texture, bool doSignal = false);

    size_t dynamicTextureWidth() const;
    size_t dynamicTextureHeight() const;

    shared_ptr<gl::BaseTexture> const reserveFontTexture(bool doWait = false);
    void freeFontTexture(shared_ptr<gl::BaseTexture> const & texture, bool doSignal = false);

    size_t fontTextureWidth() const;
    size_t fontTextureHeight() const;

    shared_ptr<GlyphInfo> const getGlyphInfo(GlyphKey const & key);
    GlyphMetrics const getGlyphMetrics(GlyphKey const & key);
    GlyphCache * glyphCache(int glyphCacheID = 0);

    int renderThreadGlyphCacheID() const;
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

