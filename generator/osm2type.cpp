#include "generator/osm2type.hpp"

#include "generator/osm2meta.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"
#include "generator/utils.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/country_tree_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_impl.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace ftype
{
using namespace std;

namespace
{
template <typename ToDo>
void ForEachTag(OsmElement * p, ToDo && toDo)
{
  for (auto & e : p->m_tags)
    toDo(move(e.m_key), move(e.m_value));
}

class NamesExtractor
{
public:
  enum class LangAction
  {
    Forbid,
    Accept,
    Append,
    Replace
  };

  explicit NamesExtractor(FeatureBuilderParams & params) : m_params(params) {}

  LangAction GetLangByKey(string const & k, string & lang)
  {
    strings::SimpleTokenizer token(k, "\t :");
    if (!token)
      return LangAction::Forbid;

    // Is this an international (latin) / old / alternative name.
    if (*token == "int_name" || *token == "old_name" || *token == "alt_name")
    {
      lang = *token;

      // Consider only pure int/alt/old name without :lang. Otherwise feature with several
      // alt_name:lang will receive random name based on tags enumeration order.
      // For old_name we support old_name:date.
      if (++token)
      {
        if (lang != "old_name")
          return LangAction::Forbid;

        if (base::AllOf(*token, [](auto const c) { return isdigit(c) || c == '-'; }))
          return LangAction::Append;

        return LangAction::Forbid;
      }

      return lang == "old_name" ? LangAction::Append : LangAction::Accept;
    }

    if (*token != "name")
      return LangAction::Forbid;

    ++token;
    lang = (token ? *token : "default");

    // Do not consider languages with suffixes, like "en:pronunciation".
    if (++token)
      return LangAction::Forbid;

    // Replace dummy arabian tag with correct tag.
    if (lang == "ar1")
      lang = "ar";

    if (lang == "ja-Hira")
    {
      // Save "ja-Hira" if there is no "ja_kana".
      lang = "ja_kana";
    }
    else if (lang == "ja_kana")
    {
      // Prefer "ja_kana" over "ja-Hira".
      return LangAction::Replace;
    }

    return LangAction::Accept;
  }

  void operator()(string && k, string && v)
  {
    if (v.empty())
      return;

    string lang;
    switch (GetLangByKey(k, lang))
    {
    case LangAction::Forbid: return;
    case LangAction::Accept: m_names.emplace(move(lang), move(v)); break;
    case LangAction::Replace: swap(m_names[lang], v); break;
    case LangAction::Append:
      auto & name = m_names[lang];
      if (name.empty())
        swap(name, v);
      else
        name = name + ";" + v;
      break;
    }
    k.clear();
    v.clear();
  }

  void Finish()
  {
    for (auto const & kv : m_names)
      m_params.AddName(kv.first, kv.second);
    m_names.clear();
  }

private:
  map<string, string> m_names;
  FeatureBuilderParams & m_params;
};

class TagProcessor
{
public:
  explicit TagProcessor(OsmElement * elem) : m_element(elem) {}

  template <typename Function>
  struct Rule
  {
    char const * m_key;
    // * - take any values
    // ! - take only negative values
    // ~ - take only positive values
    char const * m_value;
    function<Function> m_func;
  };

  template <typename Function = void()>
  void ApplyRules(initializer_list<Rule<Function>> const & rules) const
  {
    for (auto & e : m_element->m_tags)
    {
      for (auto const & rule : rules)
      {
        if (e.m_key != rule.m_key)
          continue;
        bool take = false;
        if (rule.m_value[0] == '*')
          take = true;
        else if (rule.m_value[0] == '!')
          take = IsNegative(e.m_value);
        else if (rule.m_value[0] == '~')
          take = !IsNegative(e.m_value);

        if (take || e.m_value == rule.m_value)
          Call(rule.m_func, e.m_key, e.m_value);
      }
    }
  }

protected:
  static void Call(function<void()> const & f, string &, string &) { f(); }
  static void Call(function<void(string &, string &)> const & f, string & k, string & v)
  {
    f(k, v);
  }

private:
  static bool IsNegative(string const & value)
  {
    for (char const * s : {"no", "none", "false"})
    {
      if (value == s)
        return true;
    }
    return false;
  }

  OsmElement * m_element;
};

class CachedTypes
{
public:
  enum class Type
  {
    Entrance,
    Highway,
    Address,
    OneWay,
    Private,
    Lit,
    NoFoot,
    YesFoot,
    NoBicycle,
    YesBicycle,
    BicycleBidir,
    SurfacePavedGood,
    SurfacePavedBad,
    SurfaceUnpavedGood,
    SurfaceUnpavedBad,
    HasParts,
    NoCar,
    YesCar,
    Wlan,
    RailwayStation,
    SubwayStation,
    WheelchairAny,
    WheelchairYes,
    BarrierGate,
    Toll,
    BicycleOnedir,
    Ferry,
    Count
  };

  CachedTypes()
  {
    Classificator const & c = classif();

    static map<Type, vector<string>> const kTypeToName = {
        {Type::Entrance,           {"entrance"}},
        {Type::Highway,            {"highway"}},
        {Type::Address,            {"building", "address"}},
        {Type::OneWay,             {"hwtag", "oneway"}},
        {Type::Private,            {"hwtag", "private"}},
        {Type::Lit,                {"hwtag", "lit"}},
        {Type::NoFoot,             {"hwtag", "nofoot"}},
        {Type::YesFoot,            {"hwtag", "yesfoot"}},
        {Type::NoBicycle,          {"hwtag", "nobicycle"}},
        {Type::YesBicycle,         {"hwtag", "yesbicycle"}},
        {Type::BicycleBidir,       {"hwtag", "bidir_bicycle"}},
        {Type::SurfacePavedGood,   {"psurface", "paved_good"}},
        {Type::SurfacePavedBad,    {"psurface", "paved_bad"}},
        {Type::SurfaceUnpavedGood, {"psurface", "unpaved_good"}},
        {Type::SurfaceUnpavedBad,  {"psurface", "unpaved_bad"}},
        {Type::HasParts,           {"building", "has_parts"}},
        {Type::NoCar,              {"hwtag", "nocar"}},
        {Type::YesCar,             {"hwtag", "yescar"}},
        {Type::Wlan,               {"internet_access", "wlan"}},
        {Type::RailwayStation,     {"railway", "station"}},
        {Type::SubwayStation,      {"railway", "station", "subway"}},
        {Type::WheelchairAny,      {"wheelchair"}},
        {Type::WheelchairYes,      {"wheelchair", "yes"}},
        {Type::BarrierGate,        {"barrier", "gate"}},
        {Type::Toll,               {"hwtag", "toll"}},
        {Type::BicycleOnedir,      {"hwtag", "onedir_bicycle"}},
        {Type::Ferry,              {"route", "ferry"}},
    };

    m_types.resize(static_cast<size_t>(Type::Count));
    for (auto const & kv : kTypeToName)
      m_types[static_cast<size_t>(kv.first)] = c.GetTypeByPath(kv.second);
  }

  uint32_t Get(CachedTypes::Type t) const
  {
    return m_types[static_cast<size_t>(t)];
  }

  bool IsHighway(uint32_t t) const
  {
    ftype::TruncValue(t, 1);
    return t == Get(Type::Highway);
  }

  bool IsFerry(uint32_t t) const
  {
    return t == Get(Type::Ferry);
  }

  bool IsRailwayStation(uint32_t t) const
  {
    return t == Get(Type::RailwayStation);
  }

  bool IsSubwayStation(uint32_t t) const
  {
    ftype::TruncValue(t, 3);
    return t == Get(Type::SubwayStation);
  }

private:
  buffer_vector<uint32_t, static_cast<size_t>(Type::Count)> m_types;
};

void LeaveLongestTypes(vector<generator::TypeStrings> & matchedTypes)
{
  auto const less = [](auto const & lhs, auto const & rhs) { return lhs > rhs; };

  auto const equals = [](auto const & lhs, auto const & rhs) {
    if (rhs.size() > lhs.size())
      return equal(lhs.begin(), lhs.end(), rhs.begin());
    return equal(rhs.begin(), rhs.end(), lhs.begin());
  };

  base::SortUnique(matchedTypes, less, equals);
}

void MatchTypes(OsmElement * p, FeatureBuilderParams & params, function<bool(uint32_t)> const & filterType)
{
  auto static const rules = generator::ParseMapCSS(GetPlatform().GetReader("mapcss-mapping.csv"));

  vector<generator::TypeStrings> matchedTypes;
  for (auto const & [typeString, rule] : rules)
  {
    if (rule.Matches(p->m_tags))
      matchedTypes.push_back(typeString);
  }

  LeaveLongestTypes(matchedTypes);

  for (auto const & path : matchedTypes)
  {
    uint32_t const t = classif().GetTypeByPath(path);

    if (filterType(t))
      params.AddType(t);
  }
}

string MatchCity(OsmElement const * p)
{
  static map<string, m2::RectD> const cities = {
      {"almaty", {76.7223358154, 43.1480920701, 77.123336792, 43.4299852362}},
      {"amsterdam", {4.65682983398, 52.232846171, 5.10040283203, 52.4886341706}},
      {"baires", {-58.9910888672, -35.1221551064, -57.8045654297, -34.2685661867}},
      {"bangkok", {100.159606934, 13.4363737155, 100.909423828, 14.3069694978}},
      {"barcelona", {1.94458007812, 41.2489025224, 2.29614257812, 41.5414776668}},
      {"beijing", {115.894775391, 39.588757277, 117.026367187, 40.2795256688}},
      {"berlin", {13.0352783203, 52.3051199211, 13.7933349609, 52.6963610783}},
      {"boston", {-71.2676239014, 42.2117365893, -70.8879089355, 42.521711682}},
      {"brussel", {4.2448425293, 50.761653413, 4.52499389648, 50.9497757762}},
      {"budapest", {18.7509155273, 47.3034470439, 19.423828125, 47.7023684666}},
      {"chicago", {-88.3163452148, 41.3541338721, -87.1270751953, 42.2691794924}},
      {"delhi", {76.8026733398, 28.3914003758, 77.5511169434, 28.9240352884}},
      {"dnepro", {34.7937011719, 48.339820521, 35.2798461914, 48.6056737841}},
      {"dubai", {55.01953125, 24.9337667594, 55.637512207, 25.6068559937}},
      {"ekb", {60.3588867188, 56.6622647682, 61.0180664062, 57.0287738515}},
      {"frankfurt", {8.36334228516, 49.937079757, 8.92364501953, 50.2296379179}},
      {"guangzhou", {112.560424805, 22.4313401564, 113.766174316, 23.5967112789}},
      {"hamburg", {9.75860595703, 53.39151869, 10.2584838867, 53.6820686709}},
      {"helsinki", {24.3237304688, 59.9989861206, 25.48828125, 60.44638186}},
      {"hongkong", {114.039459229, 22.1848617608, 114.305877686, 22.3983322415}},
      {"kazan", {48.8067626953, 55.6372985742, 49.39453125, 55.9153515154}},
      {"kiev", {30.1354980469, 50.2050332649, 31.025390625, 50.6599083609}},
      {"koln", {6.7943572998, 50.8445380881, 7.12669372559, 51.0810964366}},
      {"lima", {-77.2750854492, -12.3279274859, -76.7999267578, -11.7988014362}},
      {"lisboa", {-9.42626953125, 38.548165423, -8.876953125, 38.9166815364}},
      {"london", {-0.4833984375, 51.3031452592, 0.2197265625, 51.6929902115}},
      {"madrid", {-4.00451660156, 40.1536868578, -3.32885742188, 40.6222917831}},
      {"mexico", {-99.3630981445, 19.2541083164, -98.879699707, 19.5960192403}},
      {"milan", {9.02252197266, 45.341528405, 9.35760498047, 45.5813674681}},
      {"minsk", {27.2845458984, 53.777934972, 27.8393554688, 54.0271334441}},
      {"moscow", {36.9964599609, 55.3962717136, 38.1884765625, 56.1118730004}},
      {"mumbai", {72.7514648437, 18.8803004445, 72.9862976074, 19.2878132403}},
      {"munchen", {11.3433837891, 47.9981928195, 11.7965698242, 48.2530267576}},
      {"newyork", {-74.4104003906, 40.4134960497, -73.4600830078, 41.1869224229}},
      {"nnov", {43.6431884766, 56.1608472541, 44.208984375, 56.4245355509}},
      {"novosibirsk", {82.4578857422, 54.8513152597, 83.2983398438, 55.2540770671}},
      {"osaka", {134.813232422, 34.1981730963, 136.076660156, 35.119908571}},
      {"oslo", {10.3875732422, 59.7812868211, 10.9286499023, 60.0401604652}},
      {"paris", {2.09014892578, 48.6637569323, 2.70538330078, 49.0414689141}},
      {"rio", {-43.4873199463, -23.0348745407, -43.1405639648, -22.7134898498}},
      {"roma", {12.3348999023, 41.7672146942, 12.6397705078, 42.0105298189}},
      {"sanfran", {-122.72277832, 37.1690715771, -121.651611328, 38.0307856938}},
      {"santiago", {-71.015625, -33.8133843291, -70.3372192383, -33.1789392606}},
      {"saopaulo", {-46.9418334961, -23.8356009866, -46.2963867187, -23.3422558351}},
      {"seoul", {126.540527344, 37.3352243593, 127.23815918, 37.6838203267}},
      {"shanghai", {119.849853516, 30.5291450367, 122.102050781, 32.1523618947}},
      {"shenzhen", {113.790893555, 22.459263801, 114.348449707, 22.9280416657}},
      {"singapore", {103.624420166, 1.21389843409, 104.019927979, 1.45278619819}},
      {"spb", {29.70703125, 59.5231755354, 31.3110351562, 60.2725145948}},
      {"stockholm", {17.5726318359, 59.1336814082, 18.3966064453, 59.5565918857}},
      {"stuttgart", {9.0877532959, 48.7471343254, 9.29306030273, 48.8755544436}},
      {"sydney", {150.42755127, -34.3615762875, 151.424560547, -33.4543597895}},
      {"taipei", {121.368713379, 24.9312761454, 121.716156006, 25.1608229799}},
      {"tokyo", {139.240722656, 35.2186974963, 140.498657227, 36.2575628263}},
      {"warszawa", {20.7202148438, 52.0322181041, 21.3024902344, 52.4091212523}},
      {"washington", {-77.4920654297, 38.5954071994, -76.6735839844, 39.2216149801}},
      {"wien", {16.0894775391, 48.0633965378, 16.6387939453, 48.3525987075}},
  };

  m2::PointD const pt(p->m_lon, p->m_lat);

  for (auto const & city : cities)
  {
    if (city.second.IsPointInside(pt))
      return city.first;
  }
  return {};
}

string DetermineSurface(OsmElement * p)
{
  string surface;
  string smoothness;
  string surface_grade;
  bool isHighway = false;

  for (auto const & tag : p->m_tags)
  {
    if (tag.m_key == "surface")
      surface = tag.m_value;
    else if (tag.m_key == "smoothness")
      smoothness = tag.m_value;
    else if (tag.m_key == "surface:grade")
      surface_grade = tag.m_value;
    else if (tag.m_key == "highway")
      isHighway = true;
    else if (tag.m_key == "4wd_only" && (tag.m_value == "yes" || tag.m_value == "recommended"))
      return "unpaved_bad";
  }

  if (!isHighway || (surface.empty() && smoothness.empty()))
    return {};

  // According to this:
  // https://wiki.openstreetmap.org/wiki/Tag:surface=compacted
  // Surfaces by quality: asphalt, concrete, paving stones, compacted.
  static base::StringIL pavedSurfaces = {
      "asphalt",  "cobblestone",    "cobblestone:flattened", "chipseal", "compacted",
      "concrete", "concrete:lanes", "concrete:plates", "fine_gravel", "metal",
      "paved", "paving_stones", "pebblestone", "sett", "unhewn_cobblestone", "wood"
  };

  static base::StringIL badSurfaces = {
      "cobblestone", "dirt", "earth", "fine_gravel",  "grass", "gravel", "ground", "metal",
      "mud", "pebblestone", "sand", "sett", "snow", "unhewn_cobblestone", "wood", "woodchips"
  };

  static base::StringIL badSmoothness = {
      "bad",           "very_bad",       "horrible",        "very_horrible", "impassable",
      "robust_wheels", "high_clearance", "off_road_wheels", "rough"
  };

  static base::StringIL goodSmoothness = { "excellent", "good" };

  auto const Has = [](base::StringIL const & il, std::string const & v)
  {
    return base::IsExist(il, v);
  };

  bool isPaved = false;
  bool isGood = true;

  if (!surface.empty())
    isPaved = Has(pavedSurfaces, surface);
  else
    isPaved = !smoothness.empty() && Has(goodSmoothness, smoothness);

  if (!smoothness.empty())
  {
    if (smoothness == "intermediate" && !surface.empty())
      isGood = !Has(badSurfaces, surface);
    else
      isGood = !Has(badSmoothness, smoothness);

    /// @todo Hm, looks like some hack, but will not change it now ..
    if (smoothness == "bad" && !isPaved)
      isGood = true;
  }
  else if (surface_grade == "0" || surface_grade == "1")
    isGood = false;
  else if (surface_grade.empty() || surface_grade == "2")
    isGood = surface.empty() || !Has(badSurfaces, surface);

  string psurface = isPaved ? "paved_" : "unpaved_";
  psurface += isGood ? "good" : "bad";
  return psurface;
}

void PreprocessElement(OsmElement * p)
{
  bool hasLayer = false;
  char const * layer = nullptr;

  bool isSubway = false;
  bool isBus = false;
  bool isTram = false;

  TagProcessor(p).ApplyRules({
      {"bridge", "yes", [&layer] { layer = "1"; }},
      {"tunnel", "yes", [&layer] { layer = "-1"; }},
      {"layer", "*", [&hasLayer] { hasLayer = true; }},

      {"railway", "subway_entrance", [&isSubway] { isSubway = true; }},
      {"bus", "yes", [&isBus] { isBus = true; }},
      {"trolleybus", "yes", [&isBus] { isBus = true; }},
      {"tram", "yes", [&isTram] { isTram = true; }},

      /// @todo Unfortunatelly, it's not working in many cases (route=subway, transport=subway).
      /// Actually, it's better to process subways after feature types assignment.
      {"station", "subway", [&isSubway] { isSubway = true; }},
  });

  if (!hasLayer && layer)
    p->AddTag("layer", layer);

  // Tag 'city' is needed for correct selection of metro icons.
  if (isSubway && p->m_type == OsmElement::EntityType::Node)
  {
    string const city = MatchCity(p);
    if (!city.empty())
      p->AddTag("city", city);
  }

  p->AddTag("psurface", DetermineSurface(p));

  // Convert public_transport tags to the older schema.
  for (auto const & tag : p->m_tags)
  {
    if (tag.m_key == "public_transport")
    {
      if (tag.m_value == "platform" && isBus)
      {
        if (p->m_type == OsmElement::EntityType::Node)
          p->AddTag("highway", "bus_stop");
      }
      else if (tag.m_value == "stop_position" && isTram && p->m_type == OsmElement::EntityType::Node)
      {
        p->AddTag("railway", "tram_stop");
      }
      break;
    }
  }

  // Merge attraction and memorial types to predefined set of values
  p->UpdateTag("artwork_type", [](string & value) {
    if (value.empty())
      return;
    if (value == "mural" || value == "graffiti" || value == "azulejo" || value == "tilework")
      value = "painting";
    else if (value == "stone" || value == "installation")
      value = "sculpture";
    else if (value == "bust")
      value = "statue";
  });

  string const & memorialType = p->GetTag("memorial:type");
  p->UpdateTag("memorial", [&memorialType](string & value) {
    if (value.empty())
    {
      if (memorialType.empty())
        return;
      else
        value = memorialType;
    }

    if (value == "blue_plaque" || value == "stolperstein")
    {
      value = "plaque";
    }
    else if (value == "war_memorial" || value == "stele" || value == "obelisk" ||
             value == "stone" || value == "cross")
    {
      value = "sculpture";
    }
    else if (value == "bust" || value == "person")
    {
      value = "statue";
    }
  });

  p->UpdateTag("attraction", [](string & value) {
    // "specified" is a special value which means we have the "attraction" tag,
    // but its value is not "animal".
    if (!value.empty() && value != "animal")
      value = "specified";
  });

  string const kCuisineKey = "cuisine";
  auto cuisines = p->GetTag(kCuisineKey);
  if (!cuisines.empty())
  {
    strings::MakeLowerCaseInplace(cuisines);
    strings::SimpleTokenizer iter(cuisines, ",;");
    auto const collapse = [](char c, string & str) {
      auto const comparator = [c](char lhs, char rhs) { return lhs == rhs && lhs == c; };
      str.erase(unique(str.begin(), str.end(), comparator), str.end());
    };

    bool first = true;
    while (iter)
    {
      string normalized(*iter);
      strings::Trim(normalized, " ");
      collapse(' ', normalized);
      replace(normalized.begin(), normalized.end(), ' ', '_');

      if (normalized.empty())
      {
        ++iter;
        continue;
      }

      // Avoid duplication for some cuisines.
      if (normalized == "bbq" || normalized == "barbeque")
        normalized = "barbecue";
      else if (normalized == "doughnut")
        normalized = "donut";
      else if (normalized == "steak")
        normalized = "steak_house";
      else if (normalized == "coffee")
        normalized = "coffee_shop";

      if (first)
        p->UpdateTag(kCuisineKey, [&normalized](string & value) { value = normalized; });
      else
        p->AddTag(kCuisineKey, normalized);

      first = false;
      ++iter;
    }
  }

  string const kAerodromeTypeKey = "aerodrome:type";
  auto aerodromeTypes = p->GetTag(kAerodromeTypeKey);
  if (!aerodromeTypes.empty())
  {
    strings::MakeLowerCaseInplace(aerodromeTypes);
    bool first = true;
    for (auto type : strings::Tokenize<std::string>(aerodromeTypes, ",;"))
    {
      strings::Trim(type);

      if (first)
        p->UpdateTag(kAerodromeTypeKey, [&type](auto & value) { value = type; });
      else
        p->AddTag(kAerodromeTypeKey, type);

      first = false;
    }
  }

  // We replace a value of 'place' with a value of 'de: place' because most people regard
  // places names as 'de: place' defines it.
  // TODO(@m.andrianov): A better solution for the future is writing this rule in replaced_tags.txt
  // file. But syntax for this isn't supported by relace tags mechanism.
  auto const dePlace = p->GetTag("de:place");
  if (!dePlace.empty())
  {
    p->UpdateTag("place", [&](auto & value) {
      value = dePlace;
    });
  }

  // In Japan, South Korea and Turkey place=province means place=state.
  auto static const infoGetter = storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
  CHECK(infoGetter, ());
  auto static const countryTree = storage::LoadCountriesFromFile(COUNTRIES_FILE);
  CHECK(countryTree, ());
  p->UpdateTag("place", [&](string & value) {
    if (value != "province")
      return;

    std::array<storage::CountryId, 3> const provinceToStateCountries = {"Japan", "South Korea",
                                                                        "Turkey"};
    auto const pt = mercator::FromLatLon(p->m_lat, p->m_lon);
    auto const countryId = infoGetter->GetRegionCountryId(pt);
    auto const country = storage::GetTopmostParentFor(*countryTree, countryId);
    if (base::FindIf(provinceToStateCountries, [&](auto const & c) { return c == country; }) !=
        provinceToStateCountries.end())
    {
      value = "state";
    }
  });
}

void PostprocessElement(OsmElement * p, FeatureBuilderParams & params)
{
  static CachedTypes const types;

  if (!params.house.IsEmpty())
  {
    // Delete "entrance" type for house number (use it only with refs).
    // Add "address" type if we have house number but no valid types.
    if (params.PopExactType(types.Get(CachedTypes::Type::Entrance)) ||
        (params.m_types.size() == 1 && params.IsTypeExist(types.Get(CachedTypes::Type::WheelchairAny), 1)))
    {
      params.name.Clear();
      // If we have address (house name or number), we should assign valid type.
      // There are a lot of features like this in Czech Republic.
      params.AddType(types.Get(CachedTypes::Type::Address));
    }
  }

  // Process yes/no tags.
  TagProcessor(p).ApplyRules({
      {"wheelchair", "designated",
       [&params] { params.AddType(types.Get(CachedTypes::Type::WheelchairYes)); }},
      {"wifi", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::Wlan)); }},
  });

  bool highwayDone = false;
  bool subwayDone = false;
  bool railwayDone = false;
  bool ferryDone = false;

  // Get a copy of source types, because we will modify params in the loop;
  FeatureBuilderParams::Types const vTypes = params.m_types;

  for (size_t i = 0; i < vTypes.size(); ++i)
  {
    if (!highwayDone && types.IsHighway(vTypes[i]))
    {
      bool addOneway = false;
      bool noOneway = false;

      TagProcessor(p).ApplyRules({
          {"oneway", "yes", [&addOneway] { addOneway = true; }},
          {"oneway", "1", [&addOneway] { addOneway = true; }},
          {"oneway", "-1",
           [&addOneway, &params] {
             addOneway = true;
             params.SetReversedGeometry(true);
           }},
          {"oneway", "!", [&noOneway] { noOneway = true; }},
          {"junction", "roundabout", [&addOneway] { addOneway = true; }},

          {"access", "private", [&params] { params.AddType(types.Get(CachedTypes::Type::Private)); }},
          {"access", "!", [&params] { params.AddType(types.Get(CachedTypes::Type::Private)); }},

          {"barrier", "gate", [&params] { params.AddType(types.Get(CachedTypes::Type::BarrierGate)); }},

          {"lit", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::Lit)); }},
          {"toll", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::Toll)); }},

          {"foot", "!", [&params] { params.AddType(types.Get(CachedTypes::Type::NoFoot)); }},
          {"foot", "use_sidepath", [&params] { params.AddType(types.Get(CachedTypes::Type::NoFoot)); }},
          {"foot", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::YesFoot)); }},
          {"sidewalk", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::YesFoot)); }},

          {"bicycle", "!", [&params] { params.AddType(types.Get(CachedTypes::Type::NoBicycle)); }},
          {"bicycle", "use_sidepath", [&params] { params.AddType(types.Get(CachedTypes::Type::NoBicycle)); }},
          {"bicycle", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::YesBicycle)); }},
          {"cycleway", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::YesBicycle)); }},
          {"cycleway:right", "~",
           [&params] { params.AddType(types.Get(CachedTypes::Type::YesBicycle)); }},
          {"cycleway:left", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::YesBicycle)); }},
          {"oneway:bicycle", "!",
           [&params] { params.AddType(types.Get(CachedTypes::Type::BicycleBidir)); }},
          {"oneway:bicycle", "~",
           [&params] { params.AddType(types.Get(CachedTypes::Type::BicycleOnedir)); }},
          {"cycleway", "opposite",
           [&params] { params.AddType(types.Get(CachedTypes::Type::BicycleBidir)); }},

          {"motor_vehicle", "private",
           [&params] { params.AddType(types.Get(CachedTypes::Type::NoCar)); }},
          {"motor_vehicle", "!", [&params] { params.AddType(types.Get(CachedTypes::Type::NoCar)); }},
          {"motor_vehicle", "yes", [&params] { params.AddType(types.Get(CachedTypes::Type::YesCar)); }},
          {"motorcar", "private", [&params] { params.AddType(types.Get(CachedTypes::Type::NoCar)); }},
          {"motorcar", "!", [&params] { params.AddType(types.Get(CachedTypes::Type::NoCar)); }},
          {"motorcar", "yes", [&params] { params.AddType(types.Get(CachedTypes::Type::YesCar)); }},
      });

      if (addOneway && !noOneway)
        params.AddType(types.Get(CachedTypes::Type::OneWay));

      highwayDone = true;
    }

    if (!ferryDone && types.IsFerry(vTypes[i]))
    {
      bool yesMotorFerry = false;
      bool noMotorFerry = false;

      TagProcessor(p).ApplyRules({
          {"foot", "!", [&params] { params.AddType(types.Get(CachedTypes::Type::NoFoot)); }},
          {"foot", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::YesFoot)); }},
          {"ferry", "footway", [&params] { params.AddType(types.Get(CachedTypes::Type::YesFoot)); }},
          {"ferry", "pedestrian", [&params] { params.AddType(types.Get(CachedTypes::Type::YesFoot)); }},
          {"ferry", "path", [&params] {
             params.AddType(types.Get(CachedTypes::Type::YesFoot));
             params.AddType(types.Get(CachedTypes::Type::YesBicycle));
          }},

          {"bicycle", "!", [&params] { params.AddType(types.Get(CachedTypes::Type::NoBicycle)); }},
          {"bicycle", "~", [&params] { params.AddType(types.Get(CachedTypes::Type::YesBicycle)); }},

          // Check for explicit no-tag.
          {"motor_vehicle", "!", [&noMotorFerry] { noMotorFerry = true; }},
          {"motorcar", "!", [&noMotorFerry] { noMotorFerry = true; }},

          {"motor_vehicle", "yes", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"motorcar", "yes", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "trunk", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "primary", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "secondary", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "tertiary", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "residential", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "service", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "unclassified", [&yesMotorFerry] { yesMotorFerry = true; }},
          {"ferry", "track", [&yesMotorFerry] { yesMotorFerry = true; }},
      });

      // Car routing for ferries should be explicitly defined.
      params.AddType(types.Get(!noMotorFerry && yesMotorFerry ? CachedTypes::Type::YesCar : CachedTypes::Type::NoCar));

      ferryDone = true;
    }

    /// @todo Probably, we can delete this processing because cities
    /// are matched by limit rect in MatchCity.
    if (!subwayDone && types.IsSubwayStation(vTypes[i]))
    {
      TagProcessor(p).ApplyRules({
          {"network", "London Underground", [&params] { params.SetRwSubwayType("london"); }},
          {"network", "New York City Subway", [&params] { params.SetRwSubwayType("newyork"); }},
          {"network", "Московский метрополитен", [&params] { params.SetRwSubwayType("moscow"); }},
          {"network", "Петербургский метрополитен", [&params] { params.SetRwSubwayType("spb"); }},
          {"network", "Verkehrsverbund Berlin-Brandenburg",
           [&params] { params.SetRwSubwayType("berlin"); }},
          {"network", "Минский метрополитен", [&params] { params.SetRwSubwayType("minsk"); }},

          {"network", "Київський метрополітен", [&params] { params.SetRwSubwayType("kiev"); }},
          {"operator", "КП «Київський метрополітен»",
           [&params] { params.SetRwSubwayType("kiev"); }},

          {"network", "RATP", [&params] { params.SetRwSubwayType("paris"); }},
          {"network", "Metro de Barcelona", [&params] { params.SetRwSubwayType("barcelona"); }},

          {"network", "Metro de Madrid", [&params] { params.SetRwSubwayType("madrid"); }},
          {"operator", "Metro de Madrid", [&params] { params.SetRwSubwayType("madrid"); }},

          {"network", "Metropolitana di Roma", [&params] { params.SetRwSubwayType("roma"); }},
          {"network", "ATAC", [&params] { params.SetRwSubwayType("roma"); }},
      });

      subwayDone = true;
    }

    if (!subwayDone && !railwayDone && types.IsRailwayStation(vTypes[i]))
    {
      TagProcessor(p).ApplyRules({
          {"network", "London Underground", [&params] { params.SetRwSubwayType("london"); }},
      });

      railwayDone = true;
    }
  }
}
}  // namespace

void GetNameAndType(OsmElement * p, FeatureBuilderParams & params,
                    function<bool(uint32_t)> filterType)
{
  // Stage1: Preprocess tags.
  PreprocessElement(p);

  // Stage2: Process feature name on all languages.
  NamesExtractor namesExtractor(params);
  ForEachTag(p, namesExtractor);
  namesExtractor.Finish();

  // Stage3: Process base feature tags.
  TagProcessor(p).ApplyRules<void(string &, string &)>({
      {"addr:housenumber", "*",
       [&params](string & k, string & v) {
         params.AddHouseName(v);
         k.clear();
         v.clear();
       }},
      {"addr:housename", "*",
       [&params](string & k, string & v) {
         params.AddHouseName(v);
         k.clear();
         v.clear();
       }},
      {"addr:street", "*",
       [&params](string & k, string & v) {
         params.AddStreet(v);
         k.clear();
         v.clear();
       }},
      {"addr:postcode", "*",
       [&params](string & k, string & v) {
         params.AddPostcode(v);
      }},
      {"population", "*",
       [&params](string & k, string & v) {
         // Get population rank.
         uint64_t const population = generator::osm_element::GetPopulation(v);
         if (population != 0)
           params.rank = feature::PopulationToRank(population);
       }},
      {"ref", "*",
       [&params](string & k, string & v) {
         // Get reference (we process road numbers only).
         params.ref = v;
         k.clear();
         v.clear();
       }},
      {"layer", "*",
       [&params](string & /* k */, string & v) {
         // Get layer.
         if (params.layer == feature::LAYER_EMPTY)
         {
           // atoi error value (0) should match empty layer constant.
           static_assert(feature::LAYER_EMPTY == 0);
           params.layer = atoi(v.c_str());
           params.layer = base::Clamp(params.layer, int8_t(feature::LAYER_LOW), int8_t(feature::LAYER_HIGH));
         }
       }},
  });

  // Stage4: Match tags in classificator to find feature types.
  MatchTypes(p, params, filterType);

  // Stage5: Postprocess feature types.
  PostprocessElement(p, params);

  params.FinishAddingTypes();

  // Stage6: Collect additional information about feature such as
  // hotel stars, opening hours, cuisine, ...
  ForEachTag(p, MetadataTagProcessor(params));
}
}  // namespace ftype
