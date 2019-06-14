#include "generator/regions/regions_builder.hpp"

#include "base/assert.hpp"
#include "base/thread_pool_computational.hpp"
#include "base/stl_helpers.hpp"
#include "base/thread_pool_computational.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <numeric>
#include <queue>
#include <thread>
#include <unordered_set>

namespace generator
{
namespace regions
{
RegionsBuilder::RegionsBuilder(Regions && regions, size_t threadsCount)
  : m_threadsCount(threadsCount)
{
  ASSERT(m_threadsCount != 0, ());

  m_regionsInAreaOrder = FormRegionsInAreaOrder(std::move(regions));
  m_countriesOuters = ExtractCountriesOuters(m_regionsInAreaOrder);
}

RegionsBuilder::Regions RegionsBuilder::FormRegionsInAreaOrder(Regions && regions)
{
  auto const cmp = [](Region const & l, Region const & r) { return l.GetArea() > r.GetArea(); };
  std::sort(std::begin(regions), std::end(regions), cmp);
  return std::move(regions);
}

RegionsBuilder::Regions RegionsBuilder::ExtractCountriesOuters(Regions & regions)
{
  Regions countriesOuters;

  auto const isCountry = [](Region const & region) {
    return AdminLevel::Two == region.GetAdminLevel();
  };
  std::copy_if(std::begin(regions), std::end(regions), std::back_inserter(countriesOuters),
      isCountry);

  base::EraseIf(regions, isCountry);

  return countriesOuters;
}

RegionsBuilder::Regions const & RegionsBuilder::GetCountriesOuters() const
{
  return m_countriesOuters;
}

RegionsBuilder::StringsList RegionsBuilder::GetCountryNames() const
{
  StringsList result;
  std::unordered_set<std::string> set;
  for (auto const & c : GetCountriesOuters())
  {
    auto const & name = c.GetName();
    if (set.insert(name).second)
      result.emplace_back(std::move(name));
  }

  return result;
}

Node::PtrList RegionsBuilder::MakeSelectedRegionsByCountry(Region const & outer,
                                                           Regions const & allRegions)
{
  std::vector<LevelRegion> regionsInCountry{{PlaceLevel::Country, outer}};
  for (auto const & region : allRegions)
  {
    if (outer.ContainsRect(region))
      regionsInCountry.emplace_back(GetLevel(region), region);
  }

  auto const comp = [](LevelRegion const & l, LevelRegion const & r) {
    auto const lArea = l.GetArea();
    auto const rArea = r.GetArea();
    return lArea != rArea ? lArea > rArea : l.GetRank() < r.GetRank();
  };
  std::sort(std::begin(regionsInCountry), std::end(regionsInCountry), comp);

  Node::PtrList nodes;
  nodes.reserve(regionsInCountry.size());
  for (auto && region : regionsInCountry)
    nodes.emplace_back(std::make_shared<Node>(std::move(region)));

  return nodes;
}

Node::Ptr RegionsBuilder::BuildCountryRegionTree(Region const & outer,
                                                 Regions const & allRegions)
{
  auto nodes = MakeSelectedRegionsByCountry(outer, allRegions);
  while (nodes.size() > 1)
  {
    auto itFirstNode = std::rbegin(nodes);
    auto & firstRegion = (*itFirstNode)->GetData();
    auto itCurr = itFirstNode + 1;
    for (; itCurr != std::rend(nodes); ++itCurr)
    {
      auto const & currRegion = (*itCurr)->GetData();

      if (!currRegion.ContainsRect(firstRegion) && !currRegion.Contains(firstRegion.GetCenter()))
        continue;

      auto const c = Compare(currRegion, firstRegion);
      if (c == 1)
      {
        (*itFirstNode)->SetParent(*itCurr);
        (*itCurr)->AddChild(*itFirstNode);
        break;
      }
    }

    nodes.pop_back();
  }

  return nodes.front();
}

// static
int RegionsBuilder::Compare(LevelRegion const & l, LevelRegion const & r)
{
  if (IsAreaLess(r, l) && l.Contains(r))
    return 1;
  if (IsAreaLess(l, r) && r.Contains(l))
    return -1;

  if (l.CalculateOverlapPercentage(r) < 50.0)
    return 0;

  auto const lArea = l.GetArea();
  auto const rArea = r.GetArea();
  if (0.5 * lArea >= rArea)
  {
    LOG(LDEBUG, ("Region", l.GetId(), GetRegionNotation(l), "contains partly",
                 r.GetId(), GetRegionNotation(r)));
    return 1;
  }
  if (0.5 * rArea >= lArea)
  {
    LOG(LDEBUG, ("Region", r.GetId(), GetRegionNotation(r), "contains partly",
                 l.GetId(), GetRegionNotation(l)));
    return -1;
  }

  return RelateByWeight(l, r);
}

// static
bool RegionsBuilder::IsAreaLess(Region const & l, Region const & r)
{
  constexpr auto lAreaRation = 1. + kAreaRelativeErrorPercent / 100.;
  return lAreaRation * l.GetArea() < r.GetArea();
}

// static
int RegionsBuilder::RelateByWeight(LevelRegion const & l, LevelRegion const & r)
{
  if (l.GetLevel() != PlaceLevel::Unknown && r.GetLevel() != PlaceLevel::Unknown)
  {
    if (l.GetLevel() > r.GetLevel())
      return -1;
    if (l.GetLevel() < r.GetLevel())
      return 1;
  }

  auto const lPlaceType = l.GetPlaceType();
  auto const rPlaceType = r.GetPlaceType();
  if (lPlaceType != PlaceType::Unknown && rPlaceType != PlaceType::Unknown)
  {
    if (lPlaceType > rPlaceType)
      return -1;
    if (lPlaceType < rPlaceType)
      return 1;
    // Check by admin level (administrative city (district + city) > city).
  }

  auto const lAdminLevel = l.GetAdminLevel();
  auto const rAdminLevel = r.GetAdminLevel();
  if (lAdminLevel != AdminLevel::Unknown && rAdminLevel != AdminLevel::Unknown)
  {
    if (lAdminLevel > rAdminLevel)
      return -1;
    if (lAdminLevel < rAdminLevel)
      return 1;
  }
  if (lAdminLevel != AdminLevel::Unknown)
    return 1;
  if (rAdminLevel != AdminLevel::Unknown)
    return -1;

  return 0;
}

void RegionsBuilder::ForEachCountry(CountryFn fn)
{
  for (auto const & countryName : GetCountryNames())
  {
    Regions outers;
    auto const & countries = GetCountriesOuters();
    auto const pred = [&](Region const & country) { return countryName == country.GetName(); };
    std::copy_if(std::begin(countries), std::end(countries), std::back_inserter(outers), pred);
    auto countryTrees = BuildCountryRegionTrees(outers);

    fn(countryName, countryTrees);
  }
}

Node::PtrList RegionsBuilder::BuildCountryRegionTrees(Regions const & outers)
{
  std::vector<std::future<Node::Ptr>> buildingTasks;
  {
    base::thread_pool::computational::ThreadPool threadPool(m_threadsCount);
    for (auto const & outer : outers)
    {
      auto result = threadPool.Submit(
          &RegionsBuilder::BuildCountryRegionTree, std::cref(outer), std::cref(m_regionsInAreaOrder));
      buildingTasks.emplace_back(std::move(result));
    }
  }
  std::vector<Node::Ptr> trees;
  trees.reserve(buildingTasks.size());
  std::transform(std::begin(buildingTasks), std::end(buildingTasks),
                 std::back_inserter(trees), [](auto & f) { return f.get(); });
  return trees;
}

// static
PlaceLevel RegionsBuilder::GetLevel(Region const & region)
{
  switch (region.GetPlaceType())
  {
  case PlaceType::Country:
    return PlaceLevel::Country;
  case PlaceType::State:
  case PlaceType::Province:
    return PlaceLevel::Region;
  case PlaceType::District:
  case PlaceType::County:
  case PlaceType::Municipality:
    return PlaceLevel::Subregion;
  case PlaceType::City:
  case PlaceType::Town:
  case PlaceType::Village:
  case PlaceType::Hamlet:
    return PlaceLevel::Locality;
  case PlaceType::Suburb:
    return PlaceLevel::Suburb;
  case PlaceType::Quarter:
  case PlaceType::Neighbourhood:
    return PlaceLevel::Sublocality;
  case PlaceType::IsolatedDwelling:
    return PlaceLevel::Sublocality;
  case PlaceType::Unknown:
    break;
  }

  if (region.GetAdminLevel() == AdminLevel::Two)
    return PlaceLevel::Country;

  return PlaceLevel::Unknown;
}
}  // namespace regions
}  // namespace generator
