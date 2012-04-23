#include "skin.hpp"
#include "skin_page.hpp"
#include "resource_style.hpp"
#include "resource_manager.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../std/iterator.hpp"
#include "../std/bind.hpp"
#include "../std/numeric.hpp"

#include "internal/opengl.hpp"


namespace yg
{
  Skin::Skin(shared_ptr<ResourceManager> const & resourceManager,
             Skin::TSkinPages const & pages)
    : m_pages(pages),
      m_staticPagesCount(pages.size()),
      m_resourceManager(resourceManager)
  {
    m_textPage = m_pages.size();
    addTextPages(1);

    m_dynamicPage = m_pages.size();
    addDynamicPages(1);
  }

  void Skin::addTextPages(int count)
  {
    m_pages.reserve(m_pages.size() + count);

    addClearPageFn(bind(&Skin::clearPageHandles, this, _1), 0);

    for (int i = 0; i < count; ++i)
    {
      uint8_t pipelineID = (uint8_t)m_pages.size();
      m_pages.push_back(make_shared_ptr(new SkinPage(m_resourceManager, SkinPage::EFonts, pipelineID)));
      m_pages.back()->addOverflowFn(bind(&Skin::onTextOverflow, this, pipelineID), 0);
    }
  }

  void Skin::addDynamicPages(int count)
  {
    m_pages.reserve(m_pages.size() + count);

    addClearPageFn(bind(&Skin::clearPageHandles, this, _1), 0);

    for (int i = 0; i < count; ++i)
    {
      uint8_t pipelineID = (uint8_t)m_pages.size();
      m_pages.push_back(make_shared_ptr(new SkinPage(m_resourceManager, SkinPage::EPrimary, pipelineID)));
      m_pages.back()->addOverflowFn(bind(&Skin::onDynamicOverflow, this, pipelineID), 0);
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

  ResourceStyle const * Skin::fromID(uint32_t id)
  {
    if (id == invalidHandle())
      return 0;

    id_pair_t p = unpackID(id);
    if (p.first < m_pages.size())
      return m_pages[p.first]->fromID(p.second);
    else
      return m_additionalPages[p.first - m_pages.size()]->fromID(p.second);
  }

  uint32_t Skin::mapSymbol(char const * symbolName)
  {
    for (uint8_t i = 0; i < m_additionalPages.size(); ++i)
    {
      uint32_t res = m_additionalPages[i]->findSymbol(symbolName);
      if (res != invalidPageHandle())
        return packID(i + m_pages.size(), res);
    }

    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      uint32_t res = m_pages[i]->findSymbol(symbolName);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    return invalidHandle();
  }

  uint32_t Skin::mapColor(Color const & c)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_additionalPages.size(); ++i)
    {
      res = m_additionalPages[i]->findColor(c);
      if (res != invalidPageHandle())
        return packID(i + m_pages.size(), res);
    }

    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      res = m_pages[i]->findColor(c);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_pages[m_dynamicPage]->hasRoom(c))
      flushDynamicPage();

    return packID(m_dynamicPage, m_pages[m_dynamicPage]->mapColor(c));
  }

  uint32_t Skin::mapPenInfo(PenInfo const & penInfo)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_additionalPages.size(); ++i)
    {
      res = m_additionalPages[i]->findPenInfo(penInfo);
      if (res != invalidPageHandle())
        return packID(i + m_pages.size(), res);
    }

    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      res = m_pages[i]->findPenInfo(penInfo);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_pages[m_dynamicPage]->hasRoom(penInfo))
      flushDynamicPage();

    return packID(m_dynamicPage, m_pages[m_dynamicPage]->mapPenInfo(penInfo));
  }

  uint32_t Skin::mapCircleInfo(CircleInfo const & circleInfo)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_additionalPages.size(); ++i)
    {
      res = m_additionalPages[i]->findCircleInfo(circleInfo);
      if (res != invalidPageHandle())
        return packID(i + m_pages.size(), res);
    }

    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      res = m_pages[i]->findCircleInfo(circleInfo);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_pages[m_dynamicPage]->hasRoom(circleInfo))
      flushDynamicPage();

    return packID(m_dynamicPage, m_pages[m_dynamicPage]->mapCircleInfo(circleInfo));
  }

  bool Skin::mapPenInfo(PenInfo const * penInfos, uint32_t * styleIDS, size_t count)
  {
    bool alreadyFlushed = false;

    int i = 0;

    do
    {
      styleIDS[i] = m_pages[m_dynamicPage]->findPenInfo(penInfos[i]);

      if ((styleIDS[i] == invalidPageHandle()) || (unpackID(styleIDS[i]).first != m_dynamicPage))
      {
        /// try to pack on the currentDynamicPage
        while (!m_pages[m_dynamicPage]->hasRoom(penInfos[i]))
        {
          /// no room - flush the page
          flushDynamicPage();
          if (alreadyFlushed)
            return false; //<cycling

          alreadyFlushed = true;
          /// re-start packing
          i = 0;
        }

        styleIDS[i] = packID(m_dynamicPage, m_pages[m_dynamicPage]->mapPenInfo(penInfos[i]));
      }

      ++i;
    }
    while (i != count);

    return true;
  }

  uint32_t Skin::mapGlyph(GlyphKey const & gk, GlyphCache * glyphCache)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_additionalPages.size(); ++i)
    {
      res = m_additionalPages[i]->findGlyph(gk);
      if (res != invalidPageHandle())
        return packID(i + m_pages.size(), res);
    }

    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      res = m_pages[i]->findGlyph(gk);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_pages[m_textPage]->hasRoom(gk, glyphCache))
      flushTextPage();

    return packID(m_textPage, m_pages[m_textPage]->mapGlyph(gk, glyphCache));
  }

  shared_ptr<SkinPage> const & Skin::getPage(int i) const
  {
    if (i < m_pages.size())
      return m_pages[i];
    else
      return m_additionalPages[i - m_pages.size()];
  }

  size_t Skin::getPagesCount() const
  {
    return m_pages.size();
  }

  size_t Skin::getAdditionalPagesCount() const
  {
    return m_additionalPages.size();
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

  void Skin::addOverflowFn(overflowFn fn, int priority)
  {
    m_overflowFns.push(std::pair<size_t, overflowFn>(priority, fn));
  }

  void Skin::callOverflowFns(uint8_t pipelineID)
  {
    overflowFns handlersCopy = m_overflowFns;
    while (!handlersCopy.empty())
    {
      handlersCopy.top().second(pipelineID);
      handlersCopy.pop();
    }
  }

  void Skin::clearPageHandles(uint8_t pipelineID)
  {
    getPage(pipelineID)->clearHandles();
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

  void Skin::flushDynamicPage()
  {
    callClearPageFns(m_dynamicPage);
  }

  void Skin::flushTextPage()
  {
    //callOverflowFns(m_currentTextPage);
    callClearPageFns(m_textPage);
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

  void Skin::setAdditionalPage(shared_ptr<SkinPage> const & page)
  {
    m_additionalPages.clear();
    m_additionalPages.push_back(page);
    m_additionalPages[0]->setPipelineID(0 + m_pages.size());
  }

  void Skin::clearAdditionalPage()
  {
    m_additionalPages.clear();
  }
}
