#pragma once

#include "../geometry/rect2d.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"
#include "../std/vector.hpp"
#include "../std/queue.hpp"

#include "resource.hpp"

namespace graphics
{
  template <typename pair_t>
  struct first_less
  {
    bool operator()(pair_t const & first, pair_t const & second)
    {
      return first.first < second.first;
    }
  };

  namespace gl
  {
    class BaseTexture;
  }

  class ResourceCache;
  class ResourceManager;

  class Skin
  {
  public:

    typedef vector<shared_ptr<ResourceCache> > TResourceCaches;
    typedef function<void(uint8_t)> clearPageFn;
    typedef function<void(uint8_t)> overflowFn;

  private:

    TResourceCaches m_caches;

    uint8_t m_startDynamicPage;
    uint8_t m_dynamicPage;
    uint8_t m_dynamicPagesCount;

    uint8_t m_textPage;

    uint8_t m_startStaticPage;
    uint8_t m_staticPagesCount;

    shared_ptr<ResourceManager> m_resourceManager;

    Skin(shared_ptr<ResourceManager> const & resourceManager,
         TResourceCaches const & pages);

    friend class SkinLoader;

    void addDynamicPages(int count);
    void addTextPages(int count);

    typedef pair<uint8_t, uint32_t> id_pair_t;
    id_pair_t unpackID(uint32_t id) const;
    uint32_t packID(uint8_t, uint32_t) const;

    typedef priority_queue<pair<size_t, clearPageFn>,
                           vector<pair<size_t, clearPageFn> >,
                           first_less<pair<size_t, clearPageFn> >
                           > clearPageFns;

    clearPageFns m_clearPageFns;
    void callClearPageFns(uint8_t pipelineID);

    typedef priority_queue<pair<size_t, overflowFn>,
                           vector<pair<size_t, overflowFn> >,
                           first_less<pair<size_t, overflowFn> >
                           > overflowFns;

    overflowFns m_overflowFns;
    void callOverflowFns(uint8_t pipelineID);

    void clearPageHandles(uint8_t pipelineID);

    bool isDynamicPage(int i) const;
    void flushDynamicPage();
    int  nextDynamicPage() const;
    void changeDynamicPage();

    void onDynamicOverflow(uint8_t pipelineID);

    bool isTextPage(int i) const;
    void flushTextPage();

    void onTextOverflow(uint8_t pipelineID);

  public:

    /// clean and destroy
    ~Skin();
    /// obtain Resource from id
    Resource const * fromID(uint32_t id);

    /// map Resource::Info on skin
    /// if found - return id.
    /// if not - pack and return id.
    uint32_t map(Resource::Info const & info);
    /// map array of Resource::Info's on skin
    bool map(Resource::Info const * const * infos, uint32_t * ids, size_t count);

    uint32_t findInfo(Resource::Info const & info);

    /// adding function which will be called, when some SkinPage
    /// is getting cleared.
    void addClearPageFn(clearPageFn fn, int priority);

    shared_ptr<ResourceCache> const & page(int i) const;

    size_t pagesCount() const;

    uint32_t invalidHandle() const;
    uint32_t invalidPageHandle() const;

    uint8_t textPage() const;
    uint8_t dynamicPage() const;

    /// change page for its "backbuffer" counterpart.
    /// this function is called after any rendering command
    /// issued to avoid the "GPU is waiting on texture used in
    /// rendering call" issue.
    /// @warning does nothing for static pages
    /// (pages loaded at skin creation time)
    /// and text pages.
    void changePage(int i);
    int nextPage(int i) const;

    void clearHandles();

    void memoryWarning();
    void enterBackground();
    void enterForeground();
  };
}
