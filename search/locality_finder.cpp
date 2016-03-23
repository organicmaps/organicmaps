#include "search/locality_finder.hpp"
#include "search/v2/mwm_context.hpp"

#include "indexer/ftypes_matcher.hpp"


namespace search
{

double const kMaxCityRadiusMeters = 30000.0;
double const kInflateRectMercator = 1.0E-3;

class DoLoader
{
public:
  DoLoader(LocalityFinder const & finder, LocalityFinder::Cache & cache)
    : m_finder(finder), m_cache(cache)
  {
  }

  void operator() (FeatureType & ft) const
  {
    if (ft.GetFeatureType() != feature::GEOM_POINT)
      return;

    using namespace ftypes;
    switch (IsLocalityChecker::Instance().GetType(ft))
    {
    case CITY:
    case TOWN:
      break;
    default:  // cache only cities and towns at this moment
      return;
    }

    uint32_t const id = ft.GetID().m_index;

    if (m_cache.m_loaded.count(id) > 0)
      return;

    uint32_t const population = ftypes::GetPopulation(ft);
    if (population == 0)
      return;

    double const radius = ftypes::GetRadiusByPopulation(population);
    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(ft.GetCenter(), radius);
    if (!rect.IsIntersect(m_cache.m_rect))
      return;

    // read item
    string name;
    if (!ft.GetName(m_finder.m_lang, name))
      if (!ft.GetName(0, name))
        return;

    LocalityItem item(population, id, name);
    m_cache.m_tree.Add(item, rect);
    m_cache.m_loaded.insert(id);
  }

private:
  LocalityFinder const & m_finder;
  LocalityFinder::Cache & m_cache;
};


class DoSelectLocality
{
public:
  DoSelectLocality(string & name, m2::PointD const & p)
    : m_name(name) , m_point(p), m_bestValue(numeric_limits<double>::max())
  {
  }

  void operator() (m2::RectD const & rect, LocalityItem const & item)
  {
    double const d = MercatorBounds::DistanceOnEarth(rect.Center(), m_point);
    double const value = ftypes::GetPopulationByRadius(d) / static_cast<double>(item.m_population);
    if (value < m_bestValue)
    {
      m_bestValue = value;
      m_name = item.m_name;
    }
  }

private:
  string & m_name;
  m2::PointD m_point;
  double m_bestValue;
};


LocalityItem::LocalityItem(uint32_t population, ID id, string const & name)
  : m_name(name), m_population(population), m_id(id)
{
}

string DebugPrint(LocalityItem const & item)
{
  stringstream ss;
  ss << "Name = " << item.m_name << "Population = " << item.m_population << "ID = " << item.m_id;
  return ss.str();
}


LocalityFinder::LocalityFinder(Index const * pIndex)
  : m_pIndex(pIndex), m_lang(0)
{
}

void LocalityFinder::UpdateCache(Cache & cache, m2::PointD const & pt) const
{
  m2::RectD rect = MercatorBounds::RectByCenterXYAndSizeInMeters(pt, kMaxCityRadiusMeters);
  if (cache.m_rect.IsRectInside(rect))
    return;

  rect.Add(cache.m_rect);
  rect.Inflate(kInflateRectMercator, kInflateRectMercator);

  vector<shared_ptr<MwmInfo>> mwmsInfo;
  m_pIndex->GetMwmsInfo(mwmsInfo);
  for (auto const & info : mwmsInfo)
  {
    Index::MwmHandle handle = m_pIndex->GetMwmHandleById(info);
    MwmValue const * value = handle.GetValue<MwmValue>();
    if (handle.IsAlive() && value->GetHeader().GetType() == feature::DataHeader::world)
    {
      cache.m_rect = rect;
      v2::MwmContext(move(handle)).ForEachFeature(rect, DoLoader(*this, cache));
      break;
    }
  }
}

void LocalityFinder::GetLocality(m2::PointD const & pt, string & name)
{
  Cache * working = nullptr;

  // Find suitable cache that includes needed point.
  for (auto & cache : m_caches)
  {
    if (cache.m_rect.IsPointInside(pt))
    {
      working = &cache;
      break;
    }
  }

  if (working == nullptr)
  {
    // Find most unused cache.
    size_t minUsage = numeric_limits<size_t>::max();
    for (auto & cache : m_caches)
    {
      if (cache.m_usage < minUsage)
      {
        working = &cache;
        minUsage = cache.m_usage;
      }
    }

    ASSERT(working, ());
    working->Clear();
  }

  UpdateCache(*working, pt);
  working->GetLocality(pt, name);
}

void LocalityFinder::ClearCache()
{
  for (auto & cache : m_caches)
    cache.Clear();
}

void LocalityFinder::Cache::Clear()
{
  m_usage = 0;
  m_tree.Clear();
  m_loaded.clear();
  m_rect.MakeEmpty();
}

void LocalityFinder::Cache::GetLocality(m2::PointD const & pt, string & name)
{
  ++m_usage;
  m_tree.ForEachInRectEx(m2::RectD(pt, pt), DoSelectLocality(name, pt));
}

}
