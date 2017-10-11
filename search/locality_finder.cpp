#include "search/locality_finder.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/dummy_rank_table.hpp"
#include "search/mwm_context.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <vector>

using namespace std;

namespace search
{
namespace
{
double const kMaxCityRadiusMeters = 30000.0;
double const kMaxVillageRadiusMeters = 2000.0;

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

class LocalitiesLoader
{
public:
  LocalitiesLoader(MwmContext const & ctx, Filter const & filter, LocalityFinder::Holder & holder,
                   map<MwmSet::MwmId, unordered_set<uint32_t>> & loadedIds)
    : m_ctx(ctx)
    , m_filter(filter)
    , m_holder(holder)
    , m_loadedIds(loadedIds[m_ctx.GetId()])
  {
  }

  void operator()(uint32_t id) const
  {
    if (!m_filter.IsGood(id))
      return;

    if (m_loadedIds.count(id) != 0)
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
    default:
      return;
    }

    auto const population = ftypes::GetPopulation(ft);
    if (population == 0)
      return;

    StringUtf8Multilang names;
    ft.ForEachName([&](int8_t lang, string const & name) {
      names.AddString(lang, name);
      return true;
    });
    auto const center = ft.GetCenter();

    m_holder.Add(LocalityItem(names, center, population));
    m_loadedIds.insert(id);
  }

private:
  MwmContext const & m_ctx;
  Filter const & m_filter;

  LocalityFinder::Holder & m_holder;
  unordered_set<uint32_t> & m_loadedIds;
};
}  // namespace

// LocalityItem ------------------------------------------------------------------------------------
LocalityItem::LocalityItem(StringUtf8Multilang const & names, m2::PointD const & center,
                           uint64_t population)
  : m_names(names), m_center(center), m_population(population)
{
}

string DebugPrint(LocalityItem const & item)
{
  stringstream os;
  os << "Names = " << DebugPrint(item.m_names) << ", ";
  os << "Center = " << DebugPrint(item.m_center) << ", ";
  os << "Population = " << item.m_population;
  return os.str();
}

// LocalitySelector --------------------------------------------------------------------------------
LocalitySelector::LocalitySelector(m2::PointD const & p) : m_p(p) {}

void LocalitySelector::operator()(LocalityItem const & item)
{
  // TODO (@y, @m): replace this naive score by p-values on
  // multivariate Gaussian.
  double const distance = MercatorBounds::DistanceOnEarth(item.m_center, m_p);

  double const score =
      ftypes::GetPopulationByRadius(distance) / static_cast<double>(item.m_population);
  if (score < m_bestScore)
  {
    m_bestScore = score;
    m_bestLocality = &item;
  }
}

// LocalityFinder::Holder --------------------------------------------------------------------------
LocalityFinder::Holder::Holder(double radiusMeters) : m_radiusMeters(radiusMeters) {}

bool LocalityFinder::Holder::IsCovered(m2::RectD const & rect) const
{
  bool covered = false;
  m_coverage.ForEachInRect(rect, [&covered](bool) { covered = true; });
  return covered;
}

void LocalityFinder::Holder::SetCovered(m2::PointD const & p)
{
  m_coverage.Add(true, m2::RectD(p, p));
}

void LocalityFinder::Holder::Add(LocalityItem const & item)
{
  m_localities.Add(item, m2::RectD(item.m_center, item.m_center));
}

void LocalityFinder::Holder::ForEachInVicinity(m2::RectD const & rect,
                                               LocalitySelector & selector) const
{
  m_localities.ForEachInRect(rect, selector);
}

m2::RectD LocalityFinder::Holder::GetRect(m2::PointD const & p) const
{
  return MercatorBounds::RectByCenterXYAndSizeInMeters(p, m_radiusMeters);
}

m2::RectD LocalityFinder::Holder::GetDRect(m2::PointD const & p) const
{
  return MercatorBounds::RectByCenterXYAndSizeInMeters(p, 2 * m_radiusMeters);
}

void LocalityFinder::Holder::Clear()
{
  m_coverage.Clear();
  m_localities.Clear();
}

// LocalityFinder ----------------------------------------------------------------------------------
LocalityFinder::LocalityFinder(Index const & index, VillagesCache & villagesCache)
  : m_index(index)
  , m_villagesCache(villagesCache)
  , m_cities(kMaxCityRadiusMeters)
  , m_villages(kMaxVillageRadiusMeters)
  , m_mapsLoaded(false)
{
}

void LocalityFinder::ClearCache()
{
  m_ranks.reset();
  m_cities.Clear();
  m_villages.Clear();

  m_maps.Clear();
  m_worldId.Reset();
  m_mapsLoaded = false;

  m_loadedIds.clear();
}

void LocalityFinder::LoadVicinity(m2::PointD const & p, bool loadCities, bool loadVillages)
{
  UpdateMaps();

  if (loadCities)
  {
    m2::RectD const crect = m_cities.GetDRect(p);
    auto handle = m_index.GetMwmHandleById(m_worldId);
    if (handle.IsAlive())
    {
      auto const & value = *handle.GetValue<MwmValue>();
      if (!m_ranks)
        m_ranks = RankTable::Load(value.m_cont);
      if (!m_ranks)
        m_ranks = make_unique<DummyRankTable>();

      MwmContext ctx(move(handle));
      ctx.ForEachIndex(crect, LocalitiesLoader(ctx, CityFilter(*m_ranks), m_cities, m_loadedIds));
    }

    m_cities.SetCovered(p);
  }

  if (loadVillages)
  {
    m2::RectD const vrect = m_villages.GetDRect(p);
    m_maps.ForEachInRect(m2::RectD(p, p), [&](MwmSet::MwmId const & id) {
      auto handle = m_index.GetMwmHandleById(id);
      if (!handle.IsAlive())
        return;

      MwmContext ctx(move(handle));
      ctx.ForEachIndex(vrect, LocalitiesLoader(ctx, VillageFilter(ctx, m_villagesCache), m_villages,
                                               m_loadedIds));
    });

    m_villages.SetCovered(p);
  }
}

void LocalityFinder::UpdateMaps()
{
  if (m_mapsLoaded)
    return;

  vector<shared_ptr<MwmInfo>> mwmsInfo;
  m_index.GetMwmsInfo(mwmsInfo);
  for (auto const & info : mwmsInfo)
  {
    MwmSet::MwmId id(info);

    switch (info->GetType())
    {
    case MwmInfo::WORLD:
      m_worldId = id;
      break;
    case MwmInfo::COUNTRY:
      m_maps.Add(id, info->m_limitRect);
      break;
    case MwmInfo::COASTS: break;
    }
  }
  m_mapsLoaded = true;
}
}  // namespace search
