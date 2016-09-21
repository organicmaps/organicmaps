#include "search/locality_finder.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
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

struct Filter
{
public:
  virtual ~Filter() = default;
  virtual bool IsGood(uint32_t id) const = 0;
};

class CityFilter : public Filter
{
public:
  CityFilter(RankTable const & ranks) : m_ranks(ranks) {}

  // Filter overrides:
  bool IsGood(uint32_t id) const override { return m_ranks.Get(id) != 0; }

private:
  RankTable const & m_ranks;
};

class VillageFilter : public Filter
{
public:
  VillageFilter(MwmContext const & ctx, VillagesCache & villages) : m_cbv(villages.Get(ctx)) {}

  // Filter overrides:
  bool IsGood(uint32_t id) const override { return m_cbv.HasBit(id); }

private:
  CBV m_cbv;
};

class DoLoader
{
public:
  DoLoader(MwmContext const & ctx, Filter const & filter, int8_t lang,
           m4::Tree<LocalityFinder::Item> & localities,
           map<MwmSet::MwmId, unordered_set<uint32_t>> & loadedIds)
    : m_ctx(ctx), m_filter(filter), m_lang(lang), m_localities(localities), m_loadedIds(loadedIds)
  {
  }

  void operator()(uint32_t id) const
  {
    auto const & mwmId = m_ctx.GetId();
    if (m_loadedIds[mwmId].count(id) != 0)
      return;

    if (!m_filter.IsGood(id))
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
    case TOWN:
    case VILLAGE:
      break;
    default:  // cache only cities and towns at this moment
      return;
    }

    uint32_t const population = ftypes::GetPopulation(ft);
    if (population == 0)
      return;

    auto const center = ft.GetCenter();
    double const radius = ftypes::GetRadiusByPopulation(population);
    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius);

    // read item
    string name;
    if (!ft.GetName(m_lang, name) && !ft.GetName(0, name))
      return;

    LocalityFinder::Item item(name, center, population);
    m_localities.Add(item, rect);
    m_loadedIds[mwmId].insert(id);
  }

private:
  MwmContext const & m_ctx;
  Filter const & m_filter;
  int8_t const m_lang;

  m4::Tree<LocalityFinder::Item> & m_localities;
  map<MwmSet::MwmId, unordered_set<uint32_t>> & m_loadedIds;
};
}  // namespace

// LocalityFinder::Item
// ------------------------------------------------------------------------------------
LocalityFinder::Item::Item(string const & name, m2::PointD const & center, uint32_t population)
  : m_name(name), m_center(center), m_population(population)
{
}

// LocalityFinder ----------------------------------------------------------------------------------
LocalityFinder::LocalityFinder(Index const & index, VillagesCache & villagesCache)
  : m_index(index), m_villagesCache(villagesCache), m_lang(0)
{
}

void LocalityFinder::SetLanguage(int8_t lang)
{
  if (m_lang == lang)
    return;

  ClearCache();
  m_lang = lang;
}

void LocalityFinder::GetLocality(m2::PointD const & pt, string & name)
{
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(pt, kMaxCityRadiusMeters);

  bool covered = false;
  m_coverage.ForEachInRect(rect, [&covered](bool) { covered = true; });
  if (!covered)
    LoadVicinity(pt);

  m_localities.ForEachInRect(rect, LocalitySelector(name, pt));
}

void LocalityFinder::ClearCache()
{
  m_ranks.reset();
  m_coverage.Clear();
  m_localities.Clear();
  m_loadedIds.clear();
}

void LocalityFinder::LoadVicinity(m2::PointD const & pt)
{
  m2::RectD const drect =
      MercatorBounds::RectByCenterXYAndSizeInMeters(pt, 2 * kMaxCityRadiusMeters);

  vector<shared_ptr<MwmInfo>> mwmsInfo;
  m_index.GetMwmsInfo(mwmsInfo);
  for (auto const & info : mwmsInfo)
  {
    MwmSet::MwmId id(info);
    Index::MwmHandle handle = m_index.GetMwmHandleById(id);

    if (!handle.IsAlive())
      continue;

    MwmValue const & value = *handle.GetValue<MwmValue>();
    auto const & header = value.GetHeader();
    switch (header.GetType())
    {
    case feature::DataHeader::world:
    {
      if (!m_ranks)
        m_ranks = RankTable::Load(value.m_cont);
      if (!m_ranks)
        m_ranks = make_unique<DummyRankTable>();

      MwmContext ctx(move(handle));
      ctx.ForEachIndex(drect,
                       DoLoader(ctx, CityFilter(*m_ranks), m_lang, m_localities, m_loadedIds));
      break;
    }
    case feature::DataHeader::country:
      if (header.GetBounds().IsPointInside(pt))
      {
        MwmContext ctx(move(handle));
        ctx.ForEachIndex(drect, DoLoader(ctx, VillageFilter(ctx, m_villagesCache), m_lang,
                                         m_localities, m_loadedIds));
      }
      break;
    case feature::DataHeader::worldcoasts: break;
    }
  }

  m_coverage.Add(true, m2::RectD(pt, pt));
}

// LocalitySelector --------------------------------------------------------------------------------
LocalitySelector::LocalitySelector(string & name, m2::PointD const & pt)
  : m_name(name), m_pt(pt), m_bestScore(numeric_limits<double>::max())
{
}

void LocalitySelector::operator()(LocalityFinder::Item const & item)
{
  // TODO (@y, @m): replace this naive score by p-values on
  // multivariate Gaussian.
  double const distance = MercatorBounds::DistanceOnEarth(item.m_center, m_pt);

  double const score =
      ftypes::GetPopulationByRadius(distance) / static_cast<double>(item.m_population);
  if (score < m_bestScore)
  {
    m_bestScore = score;
    m_name = item.m_name;
  }
}

string DebugPrint(LocalityFinder::Item const & item)
{
  stringstream ss;
  ss << "Name = " << item.m_name << ", Center = " << DebugPrint(item.m_center)
     << ", Population = " << item.m_population;
  return ss.str();
}
}  // namespace search
