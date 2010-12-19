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

  class ResourceManager
  {
  private:

    typedef map<string, shared_ptr<gl::BaseTexture> > TStaticTextures;

    TStaticTextures m_staticTextures;

    threads::Mutex m_mutex;

    list<shared_ptr<gl::BaseTexture> > m_dynamicTextures;

    list<gl::Storage> m_storages;
    list<gl::Storage> m_smallStorages;

    gl::Storage const reserveStorageImpl(bool doWait, list<gl::Storage> & l);
    void freeStorageImpl(gl::Storage const & storage, bool doSignal, list<gl::Storage> & l);

    GlyphCache m_glyphCache;

  public:

    ResourceManager(size_t vbSize, size_t ibSize, size_t storagesCount,
                    size_t smallVBSize, size_t smallIBSize, size_t smallStoragesCount,
                    size_t texWidth, size_t texHeight, size_t texCount,
                    size_t maxGlyphCacheSize);

    shared_ptr<gl::BaseTexture> const & getTexture(string const & fileName);

    gl::Storage const reserveStorage(bool doWait = false);
    void freeStorage(gl::Storage const & storage, bool doSignal = false);

    gl::Storage const reserveSmallStorage(bool doWait = false);
    void freeSmallStorage(gl::Storage const & storage, bool doSignal = false);

    shared_ptr<gl::BaseTexture> const reserveTexture(bool doWait = false);
    void freeTexture(shared_ptr<gl::BaseTexture> const & texture, bool doSignal = false);

    shared_ptr<GlyphInfo> const getGlyph(GlyphKey const & key);
    void addFont(char const * fileName);
  };

  Skin * loadSkin(shared_ptr<ResourceManager> const & resourceManager, string const & fileName);
}

