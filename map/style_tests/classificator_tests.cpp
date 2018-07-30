#include "testing/testing.hpp"
#include "helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/feature_data.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"

using namespace std;

namespace
{
void UnitTestInitPlatform()
{
  Platform & pl = GetPlatform();
  CommandLineOptions const & options = GetTestingOptions();
  if (options.m_dataPath)
    pl.SetWritableDirForTests(options.m_dataPath);
  if (options.m_resourcePath)
    pl.SetResourceDir(options.m_resourcePath);
}
}

namespace
{
  class DoCheckConsistency
  {
    Classificator const & m_c;
  public:
    DoCheckConsistency(Classificator const & c) : m_c(c) {}
    void operator() (ClassifObject const * p, uint32_t type) const
    {
      if (p->IsDrawableAny() && !m_c.IsTypeValid(type))
        TEST(false, ("Inconsistency type", type, m_c.GetFullObjectName(type)));
    }
  };
}  // namespace

UNIT_TEST(Classificator_CheckConsistency)
{
  UnitTestInitPlatform();
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
  EGeomType m_geomType;
  int m_rules;

public:
  DoCheckStyles(Classificator const & c, EGeomType geomType, int rules)
    : m_c(c), m_geomType(geomType), m_rules(rules)
  {
  }

  void operator() (ClassifObject const * p, uint32_t type) const
  {
    if (p->IsDrawableAny())
    {
      TypesHolder holder(m_geomType);
      holder.Add(type);

      pair<int, int> const range = GetDrawableScaleRangeForRules(holder, m_rules);
      if (range.first == -1 || range.second == -1)
        LOG(LINFO, ("No styles:", type, m_c.GetFullObjectName(type)));
    }
    else if (ftype::GetLevel(type) > 1)
      LOG(LINFO, ("Type without any rules:", type, m_c.GetFullObjectName(type)));
  }
};

void ForEachObject(Classificator const & c, vector<string> const & path,
                   EGeomType geomType, int rules)
{
  uint32_t const type = c.GetTypeByPath(path);
  ClassifObject const * pObj = c.GetObject(type);

  DoCheckStyles doCheck(c, geomType, rules);
  doCheck(pObj, type);
  pObj->ForEachObjectInTree(doCheck, type);
}

void ForEachObject(Classificator const & c, string const & name,
                   EGeomType geomType, int rules)
{
  vector<string> path;
  strings::Tokenize(name, "-", MakeBackInsertFunctor(path));
  ForEachObject(c, path, geomType, rules);
}

void CheckPointStyles(Classificator const & c, string const & name)
{
  ForEachObject(c, name, GEOM_POINT, RULE_CAPTION | RULE_SYMBOL);
}

void CheckLineStyles(Classificator const & c, string const & name)
{
  ForEachObject(c, name, GEOM_LINE, RULE_PATH_TEXT);
}

}  // namespace

UNIT_TEST(Classificator_DrawingRules)
{
  UnitTestInitPlatform();
  styles::RunForEveryMapStyle([](MapStyle)
  {
    Classificator const & c = classif();

    LOG(LINFO, ("--------------- Point styles ---------------"));
    CheckPointStyles(c, "landuse");
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
    //CheckLineStyles(c, "railway");
  });
}

namespace
{

pair<int, int> GetMinMax(int level, vector<uint32_t> const & types, drule::rule_type_t ruleType)
{
  pair<int, int> res(numeric_limits<int>::max(), numeric_limits<int>::min());

  drule::KeysT keys;
  feature::GetDrawRule(types, level, feature::GEOM_AREA, keys);

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

string CombineArrT(my::StringIL const & arrT)
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

void CheckPriority(vector<my::StringIL> const & arrT, vector<size_t> const & arrI, drule::rule_type_t ruleType)
{
  UnitTestInitPlatform();
  Classificator const & c = classif();
  vector<vector<uint32_t> > types;
  vector<vector<string> > typesInfo;

  styles::RunForEveryMapStyle([&](MapStyle style)
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
        TEST_LESS(minmax.second, mm.first, ("Priority bug on zoom", level, "group", i, ":",
                                            minmaxInfo, minmax.first, minmax.second, "vs",
                                            typesInfo[i], mm.first, mm.second));
        minmax = mm;
        minmaxInfo = typesInfo[i];
      }
    }
  });
}

}  // namespace

// Check area drawing priority according to the types order below (from downmost to upmost).
// If someone is desagree with this order, please, refer to VNG :)
// natural-coastline
// place-island = natural-land
// natural-wood,scrub,heath,grassland = landuse-grass,farm,farmland,forest
// natural-water,lake = landuse-basin

UNIT_TEST(Classificator_AreaPriority)
{
  vector<my::StringIL> types =
  {
    // 0
    {"natural", "coastline"},
    // 1
    {"place", "island"}, {"natural", "land"},
    // 2
    {"natural", "wood"}, {"natural", "scrub"}, {"natural", "heath"}, {"natural", "grassland"},
    {"landuse", "grass"}, {"landuse", "farm"}, {"landuse", "farmland"}, {"landuse", "forest"},
    // ?
    //{"leisure", "park"}, {"leisure", "garden"}, - maybe next time (too tricky to do it now)
    // 3
    {"natural", "water"}, {"natural", "lake"}, {"landuse", "basin"}, {"waterway", "riverbank"}
  };

  CheckPriority(types, {1, 2, 8, 4}, drule::area);
}

UNIT_TEST(Classificator_PoiPriority)
{
  {
    vector<my::StringIL> types =
    {
      // 1
      {"amenity", "atm"},
      // 2
      {"amenity", "bank"}
    };

    CheckPriority(types, {1, 1}, drule::symbol);
  }

  {
    vector<my::StringIL> types =
    {
      // 1
      {"amenity", "bench"}, {"amenity", "shelter"},
      // 2
      {"highway", "bus_stop"}, {"amenity", "bus_station"},
      {"railway", "station"}, {"railway", "halt"}, {"railway", "tram_stop"},
    };

    CheckPriority(types, {2, 5}, drule::symbol);
  }
}

UNIT_TEST(Classificator_GetType)
{
  classificator::Load();
  Classificator const & c = classif();

  uint32_t const type1 = c.GetTypeByPath({"natural", "coastline"});
  TEST_NOT_EQUAL(0, type1, ());
  TEST_EQUAL(type1, c.GetTypeByReadableObjectName("natural-coastline"), ());

  uint32_t const type2 = c.GetTypeByPath({"amenity", "parking", "private"});
  TEST_NOT_EQUAL(0, type2, ());
  TEST_EQUAL(type2, c.GetTypeByReadableObjectName("amenity-parking-private"), ());

  TEST_EQUAL(0, c.GetTypeByPathSafe({"nonexisting", "type"}), ());
  TEST_EQUAL(0, c.GetTypeByReadableObjectName("nonexisting-type"), ());
}
