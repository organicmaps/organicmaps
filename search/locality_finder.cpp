#include "search/locality_finder.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/mwm_context.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace
{
double const kMaxCityRadiusMeters = 30000.0;
double const kInflateRectMercator = 0.001;

class DoLoader
{
public:
  DoLoader(MwmContext const & ctx, LocalityFinder::Cache & cache, RankTable const & ranks,
           int8_t lang)
    : m_ctx(ctx), m_cache(cache), m_ranks(ranks), m_lang(lang)
  {
  }

  void operator()(uint32_t id) const
  {
    if (m_ranks.Get(id) == 0)
      return;

    FeatureType ft;
    if (!m_ctx.GetFeature(id, ft))
      return;

    if (ft.GetFeatureType() != feature::GEOM_POINT)
      return;

    using namespace ftypes;
    switch (IsLocalityChecker::Instance().GetType(ft))
    {
    case CITY:
    case TOWN: break;
    default:  // cache only cities and towns at this moment
      return;
    }

    if (m_cache.m_loadedIds.count(id) > 0)
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
    if (!ft.GetName(m_lang, name) && !ft.GetName(0, name))
      return;

    LocalityItem item(name, population, id);
    m_cache.m_tree.Add(item, rect);
    m_cache.m_loadedIds.insert(id);
  }

private:
  MwmContext const & m_ctx;
  LocalityFinder::Cache & m_cache;
  RankTable const & m_ranks;
  int8_t const m_lang;
};

class DoSelectLocality
{
public:
  DoSelectLocality(string & name, m2::PointD const & pt)
    : m_name(name), m_pt(pt), m_bestScore(numeric_limits<double>::max())
  {
  }

  void operator()(m2::RectD const & rect, LocalityItem const & item)
  {
    // TODO (@y, @m): replace this naive score by p-values on
    // multivariate Gaussian.
    double const distanceMeters = MercatorBounds::DistanceOnEarth(rect.Center(), m_pt);
    double const score =
        ftypes::GetPopulationByRadius(distanceMeters) / static_cast<double>(item.m_population);
    if (score < m_bestScore)
    {
      m_bestScore = score;
      m_name = item.m_name;
    }
  }

private:
  string & m_name;
  m2::PointD m_pt;
  double m_bestScore;
};
}  // namespace

// LocalityItem ------------------------------------------------------------------------------------
LocalityItem::LocalityItem(string const & name, uint32_t population, uint32_t id)
  : m_name(name), m_population(population), m_id(id)
{
}

// LocalityFinder ----------------------------------------------------------------------------------
LocalityFinder::LocalityFinder(Index const & index) : m_index(index), m_lang(0) {}

void LocalityFinder::SetLanguage(int8_t lang)
{
  if (m_lang == lang)
    return;

  ClearCache();
  m_lang = lang;
}

void LocalityFinder::UpdateCache(Cache & cache, m2::PointD const & pt)
{
  m2::RectD rect = MercatorBounds::RectByCenterXYAndSizeInMeters(pt, kMaxCityRadiusMeters);
  if (cache.m_rect.IsRectInside(rect))
    return;

  rect.Add(cache.m_rect);
  rect.Inflate(kInflateRectMercator, kInflateRectMercator);

  if (!m_worldId.IsAlive())
  {
    m_worldId.Reset();
    m_ranks.reset();

    vector<shared_ptr<MwmInfo>> mwmsInfo;
    m_index.GetMwmsInfo(mwmsInfo);
    for (auto const & info : mwmsInfo)
    {
      MwmSet::MwmId id(info);
      Index::MwmHandle handle = m_index.GetMwmHandleById(id);
      MwmValue const * value = handle.GetValue<MwmValue>();
      if (handle.IsAlive() && value->GetHeader().GetType() == feature::DataHeader::world)
      {
        m_worldId = id;
        m_ranks = RankTable::Load(value->m_cont);
        break;
      }
    }

    if (!m_ranks)
      m_ranks = make_unique<DummyRankTable>();
  }

  ASSERT(m_ranks.get(), ());

  Index::MwmHandle handle = m_index.GetMwmHandleById(m_worldId);
  if (!handle.IsAlive())
    return;

  cache.m_rect = rect;
  MwmContext ctx(move(handle));
  ctx.ForEachIndex(rect, DoLoader(ctx, cache, *m_ranks, m_lang));
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
    working =
        &*min_element(begin(m_caches), end(m_caches), my::LessBy(&LocalityFinder::Cache::m_usage));
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

// LocalityFinder::Cache ---------------------------------------------------------------------------
void LocalityFinder::Cache::Clear()
{
  m_usage = 0;
  m_tree.Clear();
  m_loadedIds.clear();
  m_rect.MakeEmpty();
}

void LocalityFinder::Cache::GetLocality(m2::PointD const & pt, string & name)
{
  ++m_usage;
  m_tree.ForEachInRectEx(m2::RectD(pt, pt), DoSelectLocality(name, pt));
}

string DebugPrint(LocalityItem const & item)
{
  stringstream ss;
  ss << "Name = " << item.m_name << "Population = " << item.m_population << "ID = " << item.m_id;
  return ss.str();
}
}  // namespace search
