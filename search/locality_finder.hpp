#pragma once

#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include "std/set.hpp"
#include "std/unique_ptr.hpp"

class Index;

namespace search
{
struct LocalityItem
{
  LocalityItem(string const & name, uint32_t population, uint32_t id);

  string m_name;
  uint32_t m_population;
  uint32_t m_id;
};

class LocalityFinder
{
public:
  struct Cache
  {
    void Clear();
    void GetLocality(m2::PointD const & pt, string & name);

    m4::Tree<LocalityItem> m_tree;
    set<uint32_t> m_loadedIds;
    size_t m_usage = 0;
    m2::RectD m_rect;
  };

  LocalityFinder(Index const & index);

  void SetLanguage(int8_t lang);
  void GetLocality(m2::PointD const & pt, string & name);
  void ClearCache();

private:
  void UpdateCache(Cache & cache, m2::PointD const & pt);

  Index const & m_index;
  MwmSet::MwmId m_worldId;
  unique_ptr<RankTable> m_ranks;

  Cache m_caches[10];
  int8_t m_lang;
};

string DebugPrint(LocalityItem const & item);
}  // namespace search
