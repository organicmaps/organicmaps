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
  string m_name;
  uint32_t m_population;

  typedef uint32_t ID;
  ID m_id;

  LocalityItem(uint32_t population, ID id, string const & name);

  friend string DebugPrint(LocalityItem const & item);
};

class LocalityFinder
{
  struct Cache
  {
    m4::Tree<LocalityItem> m_tree;
    set<LocalityItem::ID> m_loaded;
    size_t m_usage;
    m2::RectD m_rect;

    Cache() : m_usage(0) {}

    void Clear();
    void GetLocality(m2::PointD const & pt, string & name);
  };

public:
  LocalityFinder(Index const * pIndex);

  void SetLanguage(int8_t lang)
  {
    if (m_lang != lang)
    {
      ClearCache();
      m_lang = lang;
    }
  }

  void GetLocality(m2::PointD const & pt, string & name);
  void ClearCache();

protected:
  void UpdateCache(Cache & cache, m2::PointD const & pt) const;

private:
  friend class DoLoader;

  Index const * m_pIndex;

  Cache m_caches[10];

  int8_t m_lang;
};

} // namespace search
