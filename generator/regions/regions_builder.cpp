#include "generator/regions/regions_builder.hpp"

#include "generator/regions/regions_fixer.hpp"
#include "generator/regions/specs/rus.hpp"

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
RegionsBuilder::RegionsBuilder(Regions && regions, PlacePointsMap && placePointsMap,
                               size_t threadsCount)
  : m_threadsCount(threadsCount)
{
  ASSERT(m_threadsCount != 0, ());

  MoveLabelPlacePoints(placePointsMap, regions);
  FixRegionsWithPlacePointApproximation(placePointsMap, regions);

  m_regionsInAreaOrder = FormRegionsInAreaOrder(std::move(regions));
  m_countriesOuters = ExtractCountriesOuters(m_regionsInAreaOrder);
}

void RegionsBuilder::MoveLabelPlacePoints(PlacePointsMap & placePointsMap, Regions & regions)
{
  for (auto & region : regions)
  {
    if (auto labelOsmId = region.GetLabelOsmId())
    {
      auto label = placePointsMap.find(*labelOsmId);
      if (label != placePointsMap.end())
      {
        region.SetLabel(label->second);
        placePointsMap.erase(label);
      }
    }
  }
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
    auto const placeType = region.GetPlaceType();
    if (placeType == PlaceType::Country)
      return true;

    auto const adminLevel = region.GetAdminLevel();
    return adminLevel == AdminLevel::Two && placeType == PlaceType::Unknown;
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

// static
Node::Ptr RegionsBuilder::BuildCountryRegionTree(
    Region const & outer, Regions const & regionsInAreaOrder,
    CountrySpecifier const & countrySpecifier)
{
  auto nodes = MakeCountryNodesInAreaOrder(outer, regionsInAreaOrder, countrySpecifier);

  for (auto i = std::crbegin(nodes), end = std::crend(nodes); i != end; ++i)
  {
    if (auto parent = ChooseParent(nodes, i, countrySpecifier))
    {
      (*i)->SetParent(parent);
      parent->AddChild(*i);
    }
  }

  return nodes.front();
}

// static
std::vector<Node::Ptr> RegionsBuilder::MakeCountryNodesInAreaOrder(
    Region const & countryOuter, Regions const & regionsInAreaOrder,
    CountrySpecifier const & countrySpecifier)
{
  std::vector<Node::Ptr> nodes{std::make_shared<Node>(LevelRegion{PlaceLevel::Country,
                                                                  countryOuter})};
  for (auto const & region : regionsInAreaOrder)
  {
    if (countryOuter.ContainsRect(region))
    {
      auto level = countrySpecifier.GetLevel(region);
      auto node = std::make_shared<Node>(LevelRegion{level, region});
      nodes.emplace_back(std::move(node));
    }
  }

  return nodes;
}

// static
Node::Ptr RegionsBuilder::ChooseParent(
    std::vector<Node::Ptr> const & nodesInAreaOrder,
    std::vector<Node::Ptr>::const_reverse_iterator forItem,
    CountrySpecifier const & countrySpecifier)
{
  auto const & node = *forItem;
  auto const & region = node->GetData();

  auto const from = FindAreaLowerBoundRely(nodesInAreaOrder, forItem);
  CHECK(from <= forItem, ());

  Node::Ptr parent;
  for (auto i = from, end = std::crend(nodesInAreaOrder); i != end; ++i)
  {
    auto const & candidate = *i;
    auto const & candidateRegion = candidate->GetData();

    if (parent)
    {
      auto const & parentRegion = parent->GetData();
      if (IsAreaLessRely(parentRegion, candidateRegion))
        break;
    }

    if (!candidateRegion.ContainsRect(region) && !candidateRegion.Contains(region.GetCenter()))
      continue;

    if (i == forItem)
      continue;

    auto const c = Compare(candidateRegion, region, countrySpecifier);
    if (c == 1)
    {
      if (parent && 0 <= Compare(candidateRegion, parent->GetData(), countrySpecifier))
        continue;

      parent = candidate;
    }
  }

  return parent;
}

// static
std::vector<Node::Ptr>::const_reverse_iterator RegionsBuilder::FindAreaLowerBoundRely(
    std::vector<Node::Ptr> const & nodesInAreaOrder,
    std::vector<Node::Ptr>::const_reverse_iterator forItem)
{
  auto const & region = (*forItem)->GetData();

  auto areaLessRely = [](Node::Ptr const & element, Region const & region) {
    auto const & elementRegion = element->GetData();
    return IsAreaLessRely(elementRegion, region);
  };

  return std::lower_bound(std::crbegin(nodesInAreaOrder), forItem, region, areaLessRely);
}

// static
int RegionsBuilder::Compare(LevelRegion const & l, LevelRegion const & r,
    CountrySpecifier const & countrySpecifier)
{
  if (IsAreaLessRely(r, l) && l.Contains(r))
    return 1;
  if (IsAreaLessRely(l, r) && r.Contains(l))
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

  return countrySpecifier.RelateByWeight(l, r);
}

// static
bool RegionsBuilder::IsAreaLessRely(Region const & l, Region const & r)
{
  constexpr auto lAreaRation = 1. + kAreaRelativeErrorPercent / 100.;
  return lAreaRation * l.GetArea() < r.GetArea();
}

void RegionsBuilder::ForEachCountry(CountryFn fn)
{
  for (auto const & countryName : GetCountryNames())
  {
    auto countrySpecifier = GetCountrySpecifier(countryName);

    Regions outers;
    auto const & countries = GetCountriesOuters();
    auto const pred = [&](Region const & country) { return countryName == country.GetName(); };
    std::copy_if(std::begin(countries), std::end(countries), std::back_inserter(outers), pred);
    auto countryTrees = BuildCountryRegionTrees(outers, *countrySpecifier);

    countrySpecifier->AdjustRegionsLevel(countryTrees);

    fn(countryName, countryTrees);
  }
}

Node::PtrList RegionsBuilder::BuildCountryRegionTrees(Regions const & outers,
    CountrySpecifier const & countrySpecifier)
{
  std::vector<std::future<Node::Ptr>> buildingTasks;
  {
    base::thread_pool::computational::ThreadPool threadPool(m_threadsCount);
    for (auto const & outer : outers)
    {
      auto result = threadPool.Submit(
          &RegionsBuilder::BuildCountryRegionTree,
          std::cref(outer), std::cref(m_regionsInAreaOrder), std::cref(countrySpecifier));
      buildingTasks.emplace_back(std::move(result));
    }
  }
  std::vector<Node::Ptr> trees;
  trees.reserve(buildingTasks.size());
  std::transform(std::begin(buildingTasks), std::end(buildingTasks),
                 std::back_inserter(trees), [](auto & f) { return f.get(); });
  return trees;
}

std::unique_ptr<CountrySpecifier> RegionsBuilder::GetCountrySpecifier(std::string const & countryName)
{
  if (countryName == u8"Россия" || countryName == u8"Российская Федерация" || countryName == u8"РФ")
    return std::make_unique<specs::RusSpecifier>();

  return std::make_unique<CountrySpecifier>();
}
}  // namespace regions
}  // namespace generator
