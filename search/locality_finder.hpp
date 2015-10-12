#pragma once

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include "std/set.hpp"


class Index;

namespace search
{

struct LocalityItem
{
  m2::RectD m_rect;
  string m_name;
  uint32_t m_population;

  typedef uint32_t ID;
  ID m_id;

  LocalityItem(m2::RectD const & rect, uint32_t population, ID id, string const & name);

  m2::RectD const & GetLimitRect() const { return m_rect; }
};

class LocalityFinder
{
  struct Cache
  {
    m4::Tree<LocalityItem> m_tree;
    set<LocalityItem::ID> m_loaded;
    mutable uint32_t m_usage;
    m2::RectD m_rect;

    Cache() : m_usage(0) {}

    void Clear();
    void GetLocality(m2::PointD const & pt, string & name) const;
  };

public:
  LocalityFinder(Index const * pIndex);

  void SetLanguage(int8_t lang)
  {
    if (m_lang != lang)
    {
      ClearCacheAll();
      m_lang = lang;
    }
  }

  void SetViewportByIndex(m2::RectD const & viewport, size_t idx);
  /// Set new viewport for the reserved slot only if it's no a part of the previous one.
  void SetReservedViewportIfNeeded(m2::RectD const & viewport);

  /// Check for localities in pre-cached viewports only.
  void GetLocalityInViewport(m2::PointD const & pt, string & name) const;
  /// Check for localities in all Index and make new cache if needed.
  void GetLocalityCreateCache(m2::PointD const & pt, string & name);

  void ClearCacheAll();
  void ClearCache(size_t idx);

protected:
  void CorrectMinimalRect(m2::RectD & rect) const;
  void RecreateCache(Cache & cache, m2::RectD rect) const;

private:
  friend class DoLoader;

  Index const * m_pIndex;

  enum { MAX_VIEWPORT_COUNT = 3 };
  Cache m_cache[MAX_VIEWPORT_COUNT];

  int8_t m_lang;
};

} // namespace search
