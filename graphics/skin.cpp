#include "skin.hpp"
#include "resource_cache.hpp"
#include "resource_manager.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../std/iterator.hpp"
#include "../std/bind.hpp"
#include "../std/numeric.hpp"

namespace graphics
{
  Skin::Skin(shared_ptr<ResourceManager> const & resourceManager,
             Skin::TResourceCaches const & caches)
    : m_caches(caches),
      m_staticPagesCount(caches.size()),
      m_resourceManager(resourceManager)
  {
    m_textPage = m_caches.size();
    addTextPages(1);

    m_startDynamicPage = m_dynamicPage = m_caches.size();
    m_dynamicPagesCount = 2;
    addDynamicPages(m_dynamicPagesCount);
  }

  void Skin::addTextPages(int count)
  {
    m_caches.reserve(m_caches.size() + count);

    addClearPageFn(bind(&Skin::clearPageHandles, this, _1), 0);

    for (int i = 0; i < count; ++i)
    {
      uint8_t pipelineID = (uint8_t)m_caches.size();
      m_caches.push_back(make_shared_ptr(new ResourceCache(m_resourceManager, EMediumTexture, pipelineID)));
      m_caches.back()->addOverflowFn(bind(&Skin::onTextOverflow, this, pipelineID), 0);
    }
  }

  void Skin::addDynamicPages(int count)
  {
    m_caches.reserve(m_caches.size() + count);

    addClearPageFn(bind(&Skin::clearPageHandles, this, _1), 0);

    for (int i = 0; i < count; ++i)
    {
      uint8_t pipelineID = (uint8_t)m_caches.size();
      m_caches.push_back(make_shared_ptr(new ResourceCache(m_resourceManager, ELargeTexture, pipelineID)));
      m_caches.back()->addOverflowFn(bind(&Skin::onDynamicOverflow, this, pipelineID), 0);
    }
  }

  Skin::~Skin()
  {}

  pair<uint8_t, uint32_t> Skin::unpackID(uint32_t id) const
  {
    uint8_t pipelineID = (id & 0xFF000000) >> 24;
    uint32_t h = (id & 0x00FFFFFF);
    return make_pair<uint8_t, uint32_t>(pipelineID, h);
  }

  uint32_t Skin::packID(uint8_t pipelineID, uint32_t handle) const
  {
    uint32_t pipelineIDMask = (uint32_t)pipelineID << 24;
    uint32_t h = (handle & 0x00FFFFFF);
    return (uint32_t)(pipelineIDMask | h);
  }

  Resource const * Skin::fromID(uint32_t id)
  {
    if (id == invalidHandle())
      return 0;

    id_pair_t p = unpackID(id);

    ASSERT(p.first < m_caches.size(), ());
    return m_caches[p.first]->fromID(p.second);
  }

  uint32_t Skin::map(Resource::Info const & info)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_caches.size(); ++i)
    {
      res = m_caches[i]->findInfo(info);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_caches[m_dynamicPage]->hasRoom(info))
      flushDynamicPage();

    return packID(m_dynamicPage, m_caches[m_dynamicPage]->mapInfo(info));
  }

  uint32_t Skin::findInfo(Resource::Info const & info)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_caches.size(); ++i)
    {
      res = m_caches[i]->findInfo(info);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    return res;
  }

  bool Skin::map(Resource::Info const * const * infos, uint32_t * ids, size_t count)
  {
    int startDynamicPage = m_dynamicPage;
    int cycles = 0;

    int i = 0;

    do
    {
      ids[i] = m_caches[m_dynamicPage]->findInfo(*infos[i]);

      if ((ids[i] == invalidPageHandle())
       || (unpackID(ids[i]).first != m_dynamicPage))
      {
        /// try to pack on the currentDynamicPage
        while (!m_caches[m_dynamicPage]->hasRoom(*infos[i]))
        {
          /// no room - flush the page
          flushDynamicPage();

          if (startDynamicPage == m_dynamicPage)
            cycles += 1;

          /// there could be maximum 2 cycles to
          /// pack the sequence as a whole.
          /// second cycle is necessary as the first one
          /// could possibly run on partially packed skin pages.
          if (cycles == 2)
            return false;

          /// re-start packing
          i = 0;
        }

        ids[i] = packID(m_dynamicPage, m_caches[m_dynamicPage]->mapInfo(*infos[i]));
      }

      ++i;
    }
    while (i != count);

    return true;
  }

  shared_ptr<ResourceCache> const & Skin::page(int i) const
  {
    ASSERT(i < m_caches.size(), ());
    return m_caches[i];
  }

  size_t Skin::pagesCount() const
  {
    return m_caches.size();
  }

  void Skin::addClearPageFn(clearPageFn fn, int priority)
  {
    m_clearPageFns.push(std::pair<size_t, clearPageFn>(priority, fn));
  }

  void Skin::callClearPageFns(uint8_t pipelineID)
  {
    clearPageFns handlersCopy = m_clearPageFns;
    while (!handlersCopy.empty())
    {
      handlersCopy.top().second(pipelineID);
      handlersCopy.pop();
    }
  }

  void Skin::clearPageHandles(uint8_t pipelineID)
  {
    page(pipelineID)->clearHandles();
  }

  /// This function is set to perform as a callback on texture or handles overflow
  /// BUT! Never called on texture overflow, as this situation
  /// is explicitly checked in the mapXXX() functions.
  void Skin::onDynamicOverflow(uint8_t pipelineID)
  {
    LOG(LINFO, ("DynamicPage flushing, pipelineID=", (uint32_t)pipelineID));
    flushDynamicPage();
  }

  void Skin::onTextOverflow(uint8_t pipelineID)
  {
    LOG(LINFO, ("TextPage flushing, pipelineID=", (uint32_t)pipelineID));
    flushTextPage();
  }

  bool Skin::isDynamicPage(int i) const
  {
    return (i >= m_startDynamicPage) && (i < m_startDynamicPage + m_dynamicPagesCount);
  }

  void Skin::flushDynamicPage()
  {
    callClearPageFns(m_dynamicPage);
    changeDynamicPage();
  }

  int Skin::nextDynamicPage() const
  {
    if (m_dynamicPage == m_startDynamicPage + m_dynamicPagesCount - 1)
      return m_startDynamicPage;
    else
      return m_dynamicPage + 1;
  }

  void Skin::changeDynamicPage()
  {
    m_dynamicPage = nextDynamicPage();
  }

  bool Skin::isTextPage(int i) const
  {
    return i == m_textPage;
  }

  void Skin::flushTextPage()
  {
    //callOverflowFns(m_currentTextPage);
    callClearPageFns(m_textPage);
  }

  int Skin::nextPage(int i) const
  {
    ASSERT(i < m_caches.size(), ());

    if (isDynamicPage(i))
      return nextDynamicPage();

    /// for static and text pages return same index as passed in.
    return i;
  }

  void Skin::changePage(int i)
  {
    if (isDynamicPage(i))
      changeDynamicPage();
  }

  uint32_t Skin::invalidHandle() const
  {
    return 0xFFFFFFFF;
  }

  uint32_t Skin::invalidPageHandle() const
  {
    return 0x00FFFFFF;
  }

  uint8_t Skin::textPage() const
  {
    return m_textPage;
  }

  uint8_t Skin::dynamicPage() const
  {
    return m_dynamicPage;
  }

  void Skin::memoryWarning()
  {
  }

  void Skin::enterBackground()
  {
  }

  void Skin::enterForeground()
  {
  }

  void Skin::clearHandles()
  {
    for (unsigned i = 0; i < m_caches.size(); ++i)
      m_caches[i]->clear();
  }
}
