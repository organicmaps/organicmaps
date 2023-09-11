#include "drape_frontend/stylist.hpp"
#include "drape/utils/projection.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/scales.hpp"

#include <algorithm>
#include <limits>

namespace df
{
namespace
{
/*
* The overall rendering depth range [dp::kMinDepth;dp::kMaxDepth] is divided
* into following specific depth ranges:
* FG - foreground lines and areas (buildings..), rendered on top of other geometry always,
*      even if a fg feature is layer=-10 (e.g. tunnels should be visibile over landcover and water).
* BG-top - rendered just on top of BG-by-size range, because ordering by size
*      doesn't always work with e.g. water mapped over a forest,
*      so water should be on top of other landcover always,
*      but linear waterways should be hidden beneath it.
* BG-by-size - landcover areas rendered in bbox size order, smaller areas are above larger ones.
* Still, a BG-top water area with layer=-1 should go below other landcover,
* and a layer=1 landcover area should be displayed above water,
* so BG-top and BG-by-size should share the same "layer" space.
*/

// Priority values coming from style files are expected to be separated into following ranges.
static double constexpr kBasePriorityFg = 0,
                        kBasePriorityBgTop = -1000,
                        kBasePriorityBgBySize = -2000;

// Define depth ranges boundaries to accomodate for all possible layer=* values.
// One layer space is drule::kLayerPriorityRange (1000). So each +1/-1 layer value shifts
// depth of the drule by -1000/+1000.

                        // FG depth range: [0;1000).
static double constexpr kBaseDepthFg = 0,
                        // layer=-10/10 gives an overall FG range of [-10000;11000).
                        kMaxLayeredDepthFg = kBaseDepthFg + (1 + feature::LAYER_HIGH) * drule::kLayerPriorityRange,
                        kMinLayeredDepthFg = kBaseDepthFg + feature::LAYER_LOW * drule::kLayerPriorityRange,
                        // Split the background layer space as 100 for BG-top and 900 for BG-by-size.
                        kBgTopRangeFraction = 0.1,
                        kDepthRangeBgTop = kBgTopRangeFraction * drule::kLayerPriorityRange,
                        kDepthRangeBgBySize = drule::kLayerPriorityRange - kDepthRangeBgTop,
                        // So the BG-top range is [-10100,-10000).
                        kBaseDepthBgTop = kMinLayeredDepthFg - kDepthRangeBgTop,
                        // And BG-by-size range is [-11000,-11000).
                        kBaseDepthBgBySize = kBaseDepthBgTop - kDepthRangeBgBySize,
                        // Minimum BG depth for layer=-10 is -21000.
                        kMinLayeredDepthBg = kBaseDepthBgBySize + feature::LAYER_LOW * drule::kLayerPriorityRange;

static_assert(dp::kMinDepth <= kMinLayeredDepthBg && kMaxLayeredDepthFg <= dp::kMaxDepth);
static_assert(dp::kMinDepth <= -drule::kOverlaysMaxPriority && drule::kOverlaysMaxPriority <= dp::kMaxDepth);

enum Type
{
  Line      = 1 << 0,
  Area      = 1 << 1,
  Symbol    = 1 << 2,
  Caption   = 1 << 3,
  Circle    = 1 << 4,
  PathText  = 1 << 5,
  Waymarker = 1 << 6,
  Shield    = 1 << 7,
  CountOfType = Shield + 1
};

inline drule::rule_type_t Convert(Type t)
{
  switch (t)
  {
  case Line     : return drule::line;
  case Area     : return drule::area;
  case Symbol   : return drule::symbol;
  case Caption  : return drule::caption;
  case Circle   : return drule::circle;
  case PathText : return drule::pathtext;
  case Waymarker: return drule::waymarker;
  case Shield   : return drule::shield;
  default:
    return drule::count_of_rules;
  }
}

inline bool IsTypeOf(drule::Key const & key, int flags)
{
  int currentFlag = Line;
  while (currentFlag < CountOfType)
  {
    Type const type = Type(flags & currentFlag);
    if (type != 0 && key.m_type == Convert(type))
      return true;

    currentFlag <<= 1;
  }

  return false;
}

class Aggregator
{
public:
  Aggregator(FeatureType & f, feature::GeomType const type, int const zoomLevel, size_t const keyCount)
    : m_pointStyleFound(false)
    , m_lineStyleFound(false)
    , m_captionRule({ nullptr, 0, false, false })
    , m_f(f)
    , m_geomType(type)
    , m_zoomLevel(zoomLevel)
    , m_areaDepth(0)
  {
    m_rules.reserve(keyCount);
    Init();
  }

  void AggregateKeys(drule::KeysT const & keys)
  {
    for (auto const & key : keys)
      ProcessKey(key);
  }

  void AggregateStyleFlags(drule::KeysT const & keys, bool const nameExists)
  {
    for (auto const & key : keys)
    {
      bool const isNonEmptyCaption = IsTypeOf(key, Caption) && nameExists;
      m_pointStyleFound |= (IsTypeOf(key, Symbol | Circle) || isNonEmptyCaption);
      m_lineStyleFound  |= IsTypeOf(key, Line);
    }
  }

  bool m_pointStyleFound;
  bool m_lineStyleFound;
  TRuleWrapper m_captionRule;
  buffer_vector<TRuleWrapper, 8> m_rules;

private:
  void ProcessKey(drule::Key const & key)
  {
    double depth = key.m_priority;

    if (IsTypeOf(key, Area | Line))
    {
      if (depth < kBasePriorityBgTop)
      {
        ASSERT(IsTypeOf(key, Area), (m_f.GetID()));
        ASSERT_GREATER_OR_EQUAL(depth, kBasePriorityBgBySize, (m_f.GetID()));
        // Prioritize BG-by-size areas by their bbox sizes instead of style-set priorities.
        depth = m_areaDepth;
      }
      else if (depth < kBasePriorityFg)
      {
        // Adjust BG-top features depth range so that it sits just above the BG-by-size range.
        depth = kBaseDepthBgTop + (depth - kBasePriorityBgTop) * kBgTopRangeFraction;
      }

      // Shift the depth according to layer=* value.
      // Note we don't adjust priorities of "point-styles" according to layer=*,
      // because their priorities are used for displacement logic only.
      /// @todo we might want to hide e.g. a trash bin under man_made=bridge or a bench on underground railway station?
      if (m_featureLayer != feature::LAYER_EMPTY)
      {
        depth += m_featureLayer * drule::kLayerPriorityRange;
      }
    }
    else
    {
      // Check overlays priorities range.
      ASSERT(-drule::kOverlaysMaxPriority <= depth && depth < drule::kOverlaysMaxPriority, (depth, m_f.GetID()));
    }
    // Check no features are clipped by the depth range constraints.
    ASSERT(dp::kMinDepth <= depth && depth <= dp::kMaxDepth, (depth, m_f.GetID(), m_featureLayer));

    drule::BaseRule const * const dRule = drule::rules().Find(key);
    if (dRule == nullptr)
      return;
    TRuleWrapper const rule({ dRule, static_cast<float>(depth), key.m_hatching, false });

    if (dRule->GetCaption(0) != nullptr)
    {
      // Don't add a caption rule to m_rules immediately, put aside for further processing.
      m_captionRule = rule;
    }
    else
    {
      // Lines can have zero width only if they have path symbols along.
      ASSERT(dRule->GetLine() == nullptr || dRule->GetLine()->width() > 0 || dRule->GetLine()->has_pathsym(), ());

      m_rules.push_back(rule);
    }
  }

  void Init()
  {
    m_featureLayer = m_f.GetLayer();

    if (m_geomType == feature::GeomType::Area)
    {
      // Calculate depth based on areas' bbox sizes instead of style-set priorities.
      m2::RectD const r = m_f.GetLimitRect(m_zoomLevel);
      // Raw areas' size range is about (1e-11, 3000).
      double const areaSize = r.SizeX() * r.SizeY();
      // Use log2() to have more precision distinguishing smaller areas.
      double const areaSizeCompact = std::log2(areaSize);
      // Compacted range is approx (-37;13).
      double constexpr minSize = -37,
                       maxSize = 13,
                       stretchFactor = kDepthRangeBgBySize / (maxSize - minSize);
      // Adjust the range to fit into [kBaseDepthBgBySize;kBaseDepthBgTop).
      m_areaDepth = kBaseDepthBgBySize + (maxSize - areaSizeCompact) * stretchFactor;

      ASSERT(kBaseDepthBgBySize <= m_areaDepth && m_areaDepth < kBaseDepthBgTop, (m_areaDepth, areaSize, areaSizeCompact, m_f.GetID()));
    }
  }

  FeatureType & m_f;
  feature::GeomType m_geomType;
  int const m_zoomLevel;
  int m_featureLayer;
  double m_areaDepth;
};
}  // namespace

IsHatchingTerritoryChecker::IsHatchingTerritoryChecker()
{
  Classificator const & c = classif();

  base::StringIL const arr3[] = {
    {"boundary", "protected_area", "1"},
  };
  for (auto const & sl : arr3)
    m_types.push_back(c.GetTypeByPath(sl));
  m_type3end = m_types.size();

  base::StringIL const arr2[] = {
    {"leisure", "nature_reserve"},
    {"boundary", "national_park"},
    {"landuse", "military"},
  };
  for (auto const & sl : arr2)
    m_types.push_back(c.GetTypeByPath(sl));
}

bool IsHatchingTerritoryChecker::IsMatched(uint32_t type) const
{
  // Matching with subtypes (see Stylist_IsHatching test).

  auto const iEnd3 = m_types.begin() + m_type3end;
  if (std::find(m_types.begin(), iEnd3, PrepareToMatch(type, 3)) != iEnd3)
    return true;
  return std::find(iEnd3, m_types.end(), PrepareToMatch(type, 2)) != m_types.end();
}

void CaptionDescription::Init(FeatureType & f, int8_t deviceLang, int const zoomLevel,
                              feature::GeomType const type, bool const auxCaptionExists)
{
  feature::NameParamsOut out;
  // TODO: remove forced secondary text for all lines and set it via styles for major roads and rivers only.
  // ATM even minor paths/streams/etc use secondary which makes their pathtexts take much more space.
  if (zoomLevel > scales::GetUpperWorldScale() && (auxCaptionExists || type == feature::GeomType::Line))
  {
    // Get both primary and secondary/aux names.
    f.GetPreferredNames(true /* allowTranslit */, deviceLang, out);
    m_auxText = out.secondary;
  }
  else
  {
    // Returns primary name only.
    f.GetReadableName(true /* allowTranslit */, deviceLang, out);
  }
  m_mainText = out.GetPrimary();
  ASSERT(m_auxText.empty() || !m_mainText.empty(), ("auxText without mainText"));

  uint8_t constexpr kLongCaptionsMaxZoom = 4;
  size_t constexpr kLowWorldMaxTextSize = 50;
  if (zoomLevel <= kLongCaptionsMaxZoom && m_mainText.size() > kLowWorldMaxTextSize)
  {
    m_mainText.clear();
    m_auxText.clear();
    return;
  }

  // Set max text size to avoid VB/IB overflow in rendering.
  size_t constexpr kMaxTextSize = 200;
  if (m_mainText.size() > kMaxTextSize)
    m_mainText = m_mainText.substr(0, kMaxTextSize) + "...";

  // TODO : its better to determine housenumbers minZoom once upon drules load and cache it,
  // but it'd mean a lot of housenumbers-specific logic in otherwise generic RulesHolder..
  uint8_t constexpr kHousenumbersMinZoom = 16;
  if (zoomLevel >= kHousenumbersMinZoom && (auxCaptionExists || m_mainText.empty()))
  {
    // TODO: its not obvious that a housenumber display is dependent on a secondary caption drule existance in styles.
    m_houseNumberText = f.GetHouseNumber();
    if (!m_houseNumberText.empty() && !m_mainText.empty() && m_houseNumberText.find(m_mainText) != std::string::npos)
      m_mainText.clear();
  }
}

bool InitStylist(FeatureType & f, int8_t deviceLang, int const zoomLevel, bool buildings3d, Stylist & s)
{
  feature::TypesHolder const types(f);

  if (!buildings3d && ftypes::IsBuildingPartChecker::Instance()(types) &&
      !ftypes::IsBuildingChecker::Instance()(types))
    return false;

  Classificator const & cl = classif();

  uint32_t mainOverlayType = 0;
  if (types.Size() == 1)
    mainOverlayType = *types.cbegin();
  else
  {
    // Determine main overlays type by priority. Priorities might be different across zoom levels
    // so a max value across all zooms is used to make sure main type doesn't change.
    int overlaysMaxPriority = std::numeric_limits<int>::min();
    for (uint32_t t : types)
    {
      int const priority = cl.GetObject(t)->GetMaxOverlaysPriority();
      if (priority > overlaysMaxPriority)
      {
        overlaysMaxPriority = priority;
        mainOverlayType = t;
      }
    }
  }

  auto const & hatchingChecker = IsHatchingTerritoryChecker::Instance();
  auto const geomType = types.GetGeomType();

  drule::KeysT keys;
  for (uint32_t t : types)
  {
    drule::KeysT typeKeys;
    cl.GetObject(t)->GetSuitable(zoomLevel, geomType, typeKeys);
    bool const hasHatching = hatchingChecker(t);

    for (auto & k : typeKeys)
    {
      // Take overlay drules from the main type only.
      if (t == mainOverlayType || !IsTypeOf(k, Caption | Symbol | Shield | PathText))
      {
        if (hasHatching && k.m_type == drule::area)
          k.m_hatching = true;
        keys.push_back(k);
      }
    }
  }

  feature::FilterRulesByRuntimeSelector(f, zoomLevel, keys);

  if (keys.empty())
    return false;

  // Leave only one area drule and an optional hatching drule.
  drule::MakeUnique(keys);

  s.m_isCoastline = types.Has(cl.GetCoastType());

  switch (geomType)
  {
  case feature::GeomType::Point:
    s.m_pointStyleExists = true;
    break;
  case feature::GeomType::Line :
    s.m_lineStyleExists = true;
    break;
  case feature::GeomType::Area :
    s.m_areaStyleExists = true;
    break;
  default:
    ASSERT(false, ());
    return false;
  }

  Aggregator aggregator(f, geomType, zoomLevel, keys.size());
  aggregator.AggregateKeys(keys);

  if (aggregator.m_captionRule.m_rule != nullptr)
  {
    s.m_captionDescriptor.Init(f, deviceLang, zoomLevel, geomType,
                               aggregator.m_captionRule.m_rule->GetCaption(1) != nullptr);

    if (s.m_captionDescriptor.IsNameExists())
      aggregator.m_rules.push_back(aggregator.m_captionRule);

    if (s.m_captionDescriptor.IsHouseNumberExists())
    {
      bool isGood = true;
      if (zoomLevel < scales::GetUpperStyleScale())
      {
        if (geomType == feature::GeomType::Area)
        {
          // Don't display housenumbers when an area (e.g. a building) is too small.
          m2::RectD const r = f.GetLimitRect(zoomLevel);
          isGood = std::min(r.SizeX(), r.SizeY()) > scales::GetEpsilonForHousenumbers(zoomLevel);
        }
        else
        {
          // Limit point housenumbers display to detailed zooms only (z18-).
          ASSERT_EQUAL(geomType, feature::GeomType::Point, ());
          isGood = zoomLevel >= scales::GetPointHousenumbersScale();
        }
      }

      if (isGood)
      {
        // Use building-address' caption drule to display house numbers.
        static auto const addressType = cl.GetTypeByPath({"building", "address"});
        drule::KeysT addressKeys;
        cl.GetObject(addressType)->GetSuitable(zoomLevel, geomType, addressKeys);
        if (!addressKeys.empty())
        {
          // A caption drule exists for this zoom level.
          ASSERT(addressKeys.size() == 1 && addressKeys[0].m_type == drule::caption,
                 ("building-address should contain a caption drule only"));
          drule::BaseRule const * const dRule = drule::rules().Find(addressKeys[0]);
          ASSERT(dRule != nullptr, ());
          TRuleWrapper const hnRule({ dRule, static_cast<float>(addressKeys[0].m_priority),
                                      false, true /* m_isHouseNumber*/ });
          aggregator.m_rules.push_back(hnRule);
        }
      }
    }
  }

  aggregator.AggregateStyleFlags(keys, s.m_captionDescriptor.IsNameExists() || s.m_captionDescriptor.IsHouseNumberExists());

  if (aggregator.m_pointStyleFound)
    s.m_pointStyleExists = true;
  if (aggregator.m_lineStyleFound)
    s.m_lineStyleExists = true;

  s.m_rules.swap(aggregator.m_rules);

  return true;
}

}  // namespace df
