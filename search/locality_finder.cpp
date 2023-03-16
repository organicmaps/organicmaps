#include "search/locality_finder.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/dummy_rank_table.hpp"
#include "search/mwm_context.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <vector>

namespace search
{
using namespace std;

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
  explicit CityFilter(RankTable const & ranks) : m_ranks(ranks) {}

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
  LocalitiesLoader(MwmContext const & ctx, CitiesBoundariesTable const & boundaries,
                   Filter const & filter, LocalityFinder::Holder & holder,
                   map<MwmSet::MwmId, unordered_set<uint32_t>> & loadedIds)
    : m_ctx(ctx)
    , m_boundaries(boundaries)
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

    auto ft = m_ctx.GetFeature(id);
    if (!ft)
      return;

    if (ft->GetGeomType() != feature::GeomType::Point)
      return;

    using namespace ftypes;
    switch (IsLocalityChecker::Instance().GetType(*ft))
    {
    case LocalityType::City:
    case LocalityType::Town:
    case LocalityType::Village:
      break;
    default:
      return;
    }

    auto const population = ftypes::GetPopulation(*ft);
    if (population == 0)
      return;

    auto const & names = ft->GetNames();
    auto const center = ft->GetCenter();

    CitiesBoundariesTable::Boundaries boundaries;
    auto const fid = ft->GetID();
    m_boundaries.Get(fid, boundaries);

    m_holder.Add(LocalityItem(names, center, std::move(boundaries), population, fid));
    m_loadedIds.insert(id);
  }

private:
  MwmContext const & m_ctx;
  CitiesBoundariesTable const & m_boundaries;
  Filter const & m_filter;

  LocalityFinder::Holder & m_holder;
  unordered_set<uint32_t> & m_loadedIds;
};

int GetVillagesScale()
{
  auto currentVillagesMinDrawableScale = 0;
  ftypes::IsVillageChecker::Instance().ForEachType([&currentVillagesMinDrawableScale](uint32_t type)
  {
    feature::TypesHolder th;
    th.Assign(type);
    currentVillagesMinDrawableScale = max(currentVillagesMinDrawableScale, GetMinDrawableScaleClassifOnly(th));
  });

  // Needed for backward compatibility. |kCompatibilityVillagesMinDrawableScale| should be set to
  // maximal value we have in mwms over all data versions.
  int const kCompatibilityVillagesMinDrawableScale = 13;
  ASSERT_LESS_OR_EQUAL(
      currentVillagesMinDrawableScale, kCompatibilityVillagesMinDrawableScale,
      ("Set kCompatibilityVillagesMinDrawableScale to", currentVillagesMinDrawableScale));
  return max(currentVillagesMinDrawableScale, kCompatibilityVillagesMinDrawableScale);
}
}  // namespace

// LocalityItem ------------------------------------------------------------------------------------
LocalityItem::LocalityItem(StringUtf8Multilang const & names, m2::PointD const & center,
                           Boundaries && boundaries, uint64_t population, FeatureID const & id)
  : m_names(names), m_center(center), m_boundaries(std::move(boundaries)), m_population(population), m_id(id)
{
}

string DebugPrint(LocalityItem const & item)
{
  stringstream os;
  os << "Names = " << DebugPrint(item.m_names) << ", ";
  os << "Center = " << DebugPrint(item.m_center) << ", ";
  os << "Population = " << item.m_population << ", ";
  os << "Boundaries = " << DebugPrint(item.m_boundaries);
  return os.str();
}

// LocalitySelector --------------------------------------------------------------------------------
LocalitySelector::LocalitySelector(m2::PointD const & p) : m_p(p) {}

void LocalitySelector::operator()(LocalityItem const & item)
{
  auto const inside = item.m_boundaries.HasPoint(m_p);

  // TODO (@y, @m): replace this naive score by p-values on
  // multivariate Gaussian.
  double const distance = mercator::DistanceOnEarth(item.m_center, m_p);

  // GetPopulationByRadius may return 0.
  double const score =
      (ftypes::GetPopulationByRadius(distance) + 1) / static_cast<double>(item.m_population);

  if (!inside && m_inside)
    return;

  ASSERT(inside || !m_inside, ());

  if ((inside && !m_inside) || (score < m_score))
  {
    m_inside = inside;
    m_score = score;
    m_locality = &item;
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
  return mercator::RectByCenterXYAndSizeInMeters(p, m_radiusMeters);
}

m2::RectD LocalityFinder::Holder::GetDRect(m2::PointD const & p) const
{
  return mercator::RectByCenterXYAndSizeInMeters(p, 2 * m_radiusMeters);
}

void LocalityFinder::Holder::Clear()
{
  m_coverage.Clear();
  m_localities.Clear();
}

// LocalityFinder ----------------------------------------------------------------------------------
LocalityFinder::LocalityFinder(DataSource const & dataSource,
                               CitiesBoundariesTable const & boundariesTable,
                               VillagesCache & villagesCache)
  : m_dataSource(dataSource)
  , m_boundariesTable(boundariesTable)
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
    auto handle = m_dataSource.GetMwmHandleById(m_worldId);
    if (handle.IsAlive())
    {
      auto const & value = *handle.GetValue();
      if (!m_ranks)
        m_ranks = RankTable::Load(value.m_cont, SEARCH_RANKS_FILE_TAG);
      if (!m_ranks)
        m_ranks = make_unique<DummyRankTable>();

      MwmContext ctx(move(handle));
      ctx.ForEachIndex(crect, LocalitiesLoader(ctx, m_boundariesTable, CityFilter(*m_ranks),
                                               m_cities, m_loadedIds));
    }

    m_cities.SetCovered(p);
  }

  if (loadVillages)
  {
    m2::RectD const vrect = m_villages.GetDRect(p);
    m_maps.ForEachInRect(m2::RectD(p, p), [&](MwmSet::MwmId const & id) {
      auto handle = m_dataSource.GetMwmHandleById(id);
      if (!handle.IsAlive())
        return;

      static int const scale = GetVillagesScale();
      MwmContext ctx(move(handle));
      ctx.ForEachIndex(vrect, scale,
                       LocalitiesLoader(ctx, m_boundariesTable, VillageFilter(ctx, m_villagesCache),
                                        m_villages, m_loadedIds));
    });

    m_villages.SetCovered(p);
  }
}

void LocalityFinder::UpdateMaps()
{
  if (m_mapsLoaded)
    return;

  vector<shared_ptr<MwmInfo>> mwmsInfo;
  m_dataSource.GetMwmsInfo(mwmsInfo);
  for (auto const & info : mwmsInfo)
  {
    MwmSet::MwmId id(info);

    switch (info->GetType())
    {
    case MwmInfo::WORLD: m_worldId = id; break;
    /// @todo Use fair MWM rect from CountryInfoGetter here and everywhere in search?
    /// @see MwmInfo.m_bordersRect for details.
    case MwmInfo::COUNTRY: m_maps.Add(id, info->m_bordersRect); break;
    case MwmInfo::COASTS: break;
    }
  }
  m_mapsLoaded = true;
}
}  // namespace search
