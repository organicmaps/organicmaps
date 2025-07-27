#include "helpers.hpp"
#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

namespace classificator_tests
{
using namespace std;

class DoCheckConsistency
{
  Classificator const & m_c;

public:
  explicit DoCheckConsistency(Classificator const & c) : m_c(c) {}
  void operator()(ClassifObject const * p, uint32_t type) const
  {
    if (p->IsDrawableAny() && !m_c.IsTypeValid(type))
      TEST(false, ("Inconsistency type", type, m_c.GetFullObjectName(type)));
  }
};

UNIT_TEST(Classificator_CheckConsistency)
{
  styles::RunForEveryMapStyle([](MapStyle)
  {
    Classificator const & c = classif();

    DoCheckConsistency doCheck(c);
    c.ForEachTree(doCheck);
  });
}

using namespace feature;

namespace
{

class DoCheckStyles
{
  Classificator const & m_c;
  GeomType m_geomType;
  int m_rules;

public:
  DoCheckStyles(Classificator const & c, GeomType geomType, int rules) : m_c(c), m_geomType(geomType), m_rules(rules) {}

  void operator()(ClassifObject const * p, uint32_t type) const
  {
    // We can't put TEST here, should check output manually. Or rewrite test in more sophisticated way.
    // - place=county/region are non-drawable
    // - historic=citywalls is not a Point
    // - waterway=* [tunnel] are non-drawable
    // - some highway,waterway are Point

    if (p->IsDrawableAny())
    {
      TypesHolder holder(m_geomType);
      holder.Add(type);

      pair<int, int> const range = GetDrawableScaleRangeForRules(holder, m_rules);
      if (range.first == -1 || range.second == -1)
        LOG(LWARNING, ("No styles:", m_c.GetFullObjectName(type)));
    }
    else if (ftype::GetLevel(type) > 1)
      LOG(LWARNING, ("Type without any rules:", m_c.GetFullObjectName(type)));
  }
};

void ForEachObject(Classificator const & c, vector<string_view> const & path, GeomType geomType, int rules)
{
  uint32_t const type = c.GetTypeByPath(path);
  ClassifObject const * pObj = c.GetObject(type);

  DoCheckStyles doCheck(c, geomType, rules);
  doCheck(pObj, type);
  pObj->ForEachObjectInTree(doCheck, type);
}

void ForEachObject(Classificator const & c, string const & name, GeomType geomType, int rules)
{
  ForEachObject(c, strings::Tokenize(name, "-"), geomType, rules);
}

void CheckPointStyles(Classificator const & c, string const & name)
{
  ForEachObject(c, name, GeomType::Point, RULE_CAPTION | RULE_SYMBOL);
}

void CheckLineStyles(Classificator const & c, string const & name)
{
  ForEachObject(c, name, GeomType::Line, RULE_PATH_TEXT);
}

}  // namespace

UNIT_TEST(Classificator_DrawingRules)
{
  styles::RunForEveryMapStyle([](MapStyle style)
  {
    if (style != MapStyle::MapStyleDefaultLight && style != MapStyle::MapStyleDefaultDark)
      return;

    Classificator const & c = classif();

    LOG(LINFO, ("--------------- Point styles ---------------"));
    CheckPointStyles(c, "amenity");
    CheckPointStyles(c, "historic");
    CheckPointStyles(c, "office");
    CheckPointStyles(c, "place");
    CheckPointStyles(c, "shop");
    CheckPointStyles(c, "sport");
    CheckPointStyles(c, "tourism");
    CheckPointStyles(c, "highway-bus_stop");
    CheckPointStyles(c, "highway-motorway_junction");
    CheckPointStyles(c, "railway-station");
    CheckPointStyles(c, "railway-tram_stop");
    CheckPointStyles(c, "railway-halt");

    LOG(LINFO, ("--------------- Linear styles ---------------"));
    CheckLineStyles(c, "highway");
    CheckLineStyles(c, "waterway");
    // CheckLineStyles(c, "railway");
  });
}

namespace
{

pair<int, int> GetMinMax(int level, vector<uint32_t> const & types, drule::TypeT ruleType)
{
  pair<int, int> res(numeric_limits<int>::max(), numeric_limits<int>::min());

  drule::KeysT keys;
  feature::GetDrawRule(types, level, feature::GeomType::Area, keys);

  for (size_t i = 0; i < keys.size(); ++i)
  {
    if (keys[i].m_type != ruleType)
      continue;

    if (keys[i].m_priority < res.first)
      res.first = keys[i].m_priority;
    if (keys[i].m_priority > res.second)
      res.second = keys[i].m_priority;
  }

  return res;
}

string CombineArrT(base::StringIL const & arrT)
{
  string result;
  for (auto it = arrT.begin(); it != arrT.end(); ++it)
  {
    if (it != arrT.begin())
      result.append("-");
    result.append(*it);
  }
  return result;
}

void CheckPriority(vector<base::StringIL> const & arrT, vector<size_t> const & arrI, drule::TypeT ruleType)
{
  Classificator const & c = classif();
  vector<vector<uint32_t>> types;
  vector<vector<string>> typesInfo;

  styles::RunForEveryMapStyle([&](MapStyle)
  {
    types.clear();
    typesInfo.clear();

    size_t ind = 0;
    for (size_t i = 0; i < arrI.size(); ++i)
    {
      types.push_back(vector<uint32_t>());
      types.back().reserve(arrI[i]);
      typesInfo.push_back(vector<string>());
      typesInfo.back().reserve(arrI[i]);

      for (size_t j = 0; j < arrI[i]; ++j)
      {
        types.back().push_back(c.GetTypeByPath(arrT[ind]));
        typesInfo.back().push_back(CombineArrT(arrT[ind]));
        ++ind;
      }
    }

    TEST_EQUAL(ind, arrT.size(), ());

    for (int level = scales::GetUpperWorldScale() + 1; level <= scales::GetUpperStyleScale(); ++level)
    {
      pair<int, int> minmax(numeric_limits<int>::max(), numeric_limits<int>::min());
      vector<string> minmaxInfo;
      for (size_t i = 0; i < types.size(); ++i)
      {
        pair<int, int> const mm = GetMinMax(level, types[i], ruleType);
        TEST_LESS(minmax.second, mm.first,
                  ("Priority bug on zoom", level, "group", i, ":", minmaxInfo, minmax.first, minmax.second, "vs",
                   typesInfo[i], mm.first, mm.second));
        minmax = mm;
        minmaxInfo = typesInfo[i];
      }
    }
  });
}

}  // namespace

// Check area drawing priority according to the types order below (from downmost to upmost).
// If someone disagrees with this order, please refer to VNG :)
// natural-coastline
// place-island = natural-land
// natural-scrub,heath,grassland = landuse-grass,farmland,forest
// natural-water,lake = landuse-basin

UNIT_TEST(Classificator_AreaPriority)
{
  CheckPriority(
      {// 0
       {"natural", "coastline"},
       // 1
       {"place", "island"},
       {"natural", "land"},
       // 2
       {"natural", "scrub"},
       {"natural", "heath"},
       {"natural", "grassland"},
       {"landuse", "grass"},
       {"landuse", "farmland"},
       {"landuse", "forest"},
       // ?
       //{"leisure", "park"}, {"leisure", "garden"}, - maybe next time (too tricky to do it now)
       // 3
       {"natural", "water"},
       {"natural", "water", "lake"},
       {"landuse", "basin"}},
      {1, 2, 6, 3}, drule::area);

  CheckPriority(
      {
          // ? - linear waterways @todo: add ability to compare different drule types (areas vs lines)
          //{"waterway", "river"}, {"waterway", "stream"}, {"natural", "strait"}, {"waterway", "ditch"},
          // 0 - water areas
          {"natural", "water"},
          {"landuse", "reservoir"},
          {"natural", "water", "river"},
          {"waterway", "dock"},
          // ? - hatching fills @todo: absent in vehicle style, need to test main style only
          //{"leisure", "nature_reserve"}, {"boundary", "national_park"}, {"landuse", "military"},
          // 1 - above-water features
          {"man_made", "pier"},
          {"man_made", "breakwater"},
          {"waterway", "dam"},
      },
      {4, 3}, drule::area);

  CheckPriority(
      {
          // 0
          {"leisure", "park"},
          // 1
          {"leisure", "pitch"},
          {"leisure", "playground"},
          {"sport", "multi"},
      },
      {1, 3}, drule::area);
}

UNIT_TEST(Classificator_PoiPriority)
{
  {
    CheckPriority(
        {
            // 1
            {"amenity", "drinking_water"},
            // 2
            {"tourism", "camp_site"},
            // 3
            {"tourism", "wilderness_hut"},
            // 4
            {"tourism", "alpine_hut"},
        },
        {1, 1, 1, 1}, drule::symbol);
  }
}

UNIT_TEST(Classificator_MultipleTypesPoiPriority)
{
  {
    CheckPriority(
        {// 1
         {"amenity", "atm"},
         // 2
         {"amenity", "bank"}},
        {1, 1}, drule::symbol);
  }

  {
    CheckPriority(
        {
            // 1
            {"amenity", "bench"},
            {"amenity", "shelter"},
            // 2
            {"highway", "bus_stop"},
            {"amenity", "bus_station"},
            {"railway", "station"},
            {"railway", "halt"},
            {"railway", "tram_stop"},
        },
        {2, 5}, drule::symbol);
  }

  /// @todo Check that all of sport=* icons priority is bigger than all of pitch, sports_center, recreation_ground.

  {
    CheckPriority(
        {// 1
         {"leisure", "pitch"},
         // 2
         {"sport", "yoga"}},
        {1, 1}, drule::symbol);
  }

  {
    CheckPriority(
        {// 1
         {"leisure", "sports_centre"},
         // 2
         {"sport", "shooting"}},
        {1, 1}, drule::symbol);
  }

  {
    CheckPriority(
        {// 1
         {"landuse", "recreation_ground"},
         // 2
         {"sport", "multi"}},
        {1, 1}, drule::symbol);
  }
}

namespace
{
struct RangeEntry
{
  uint32_t m_type;
  std::pair<int, int> m_range;

  // From C++ 20.
  // auto operator<=>(RangeEntry const &) const = default;
  bool operator!=(RangeEntry const & rhs) const { return m_type != rhs.m_type || m_range != rhs.m_range; }

  friend std::string DebugPrint(RangeEntry const & e)
  {
    std::ostringstream ss;
    ss << classif().GetReadableObjectName(e.m_type) << "; (" << e.m_range.first << "," << e.m_range.second << ")";
    return ss.str();
  }
};
}  // namespace

UNIT_TEST(Classificator_HighwayZoom_AcrossStyles)
{
  std::array<std::vector<RangeEntry>, MapStyleCount> scales;

  styles::RunForEveryMapStyle([&scales](MapStyle style)
  {
    auto const & cl = classif();
    uint32_t const type = cl.GetTypeByPath({"highway"});
    ClassifObject const * pObj = cl.GetObject(type);

    pObj->ForEachObjectInTree([&scales, style](ClassifObject const *, uint32_t type)
    {
      TypesHolder holder(GeomType::Line);
      holder.Add(type);
      scales[style].push_back({type, GetDrawableScaleRangeForRules(holder, RULE_LINE)});
    }, type);
  });

  for (size_t iStyle = 1; iStyle < MapStyleCount; ++iStyle)
  {
    if (iStyle == MapStyleMerged)
      continue;

    // Don't put TEST, only diagnostic logs. In general, Clear and Vehical visibility styles are different
    // for highways like: footway, path, steps, cycleway, bridleway, track, service.
    TEST_EQUAL(scales[0].size(), scales[iStyle].size(), (iStyle));
    for (size_t j = 0; j < scales[0].size(); ++j)
      if (scales[0][j] != scales[iStyle][j])
        LOG(LWARNING, (scales[0][j], scales[iStyle][j]));
  }
}
}  // namespace classificator_tests
