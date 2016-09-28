#pragma once

#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include "std/unique_ptr.hpp"
#include "std/unordered_set.hpp"

class Index;

namespace search
{
class VillagesCache;

class LocalityFinder
{
public:
  struct Item
  {
    Item(string const & name, m2::PointD const & center, uint32_t population);

    string m_name;
    m2::PointD m_center;
    uint32_t m_population;
  };

  LocalityFinder(Index const & index, VillagesCache & villagesCache);

  void SetLanguage(int8_t lang);
  void GetLocality(m2::PointD const & pt, string & name);
  void ClearCache();

private:
  void LoadVicinity(m2::PointD const & pt);

  Index const & m_index;
  VillagesCache & m_villagesCache;
  unique_ptr<RankTable> m_ranks;

  m4::Tree<bool> m_coverage;
  m4::Tree<Item> m_localities;
  map<MwmSet::MwmId, unordered_set<uint32_t>> m_loadedIds;

  int8_t m_lang;
};

class LocalitySelector
{
public:
  LocalitySelector(string & name, m2::PointD const & pt);

  void operator()(LocalityFinder::Item const & item);

private:
  string & m_name;
  m2::PointD m_pt;
  double m_bestScore;
};

string DebugPrint(LocalityFinder::Item const & item);
}  // namespace search
