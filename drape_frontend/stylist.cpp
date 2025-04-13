#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include <algorithm>
#include <limits>

namespace df
{
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
    {"boundary", "aboriginal_lands"},
    {"leisure", "nature_reserve"},
    {"boundary", "national_park"},
    {"landuse", "military"},
    {"amenity", "prison"},
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

void CaptionDescription::Init(FeatureType & f, int8_t deviceLang, int zoomLevel,
                              feature::GeomType geomType, bool auxCaptionExists)
{
  feature::NameParamsOut out;
  // TODO(pastk) : remove forced secondary text for all lines and set it via styles for major roads and rivers only.
  // ATM even minor paths/streams/etc use secondary which makes their pathtexts take much more space.
  if (zoomLevel > scales::GetUpperWorldScale() && (auxCaptionExists || geomType == feature::GeomType::Line))
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

  // TODO(pastk) : its better to determine housenumbers minZoom once upon drules load and cache it,
  // but it'd mean a lot of housenumbers-specific logic in otherwise generic RulesHolder..
  uint8_t constexpr kHousenumbersMinZoom = 16;
  if (geomType != feature::GeomType::Line && zoomLevel >= kHousenumbersMinZoom &&
      (auxCaptionExists || m_mainText.empty()))
  {
    // TODO(pastk) : its not obvious that a housenumber display is dependent on a secondary caption drule existance in styles.
    m_houseNumberText = f.GetHouseNumber();
    if (!m_houseNumberText.empty() && !m_mainText.empty() && m_houseNumberText.find(m_mainText) != std::string::npos)
      m_mainText.clear();
  }
}

void Stylist::ProcessKey(FeatureType & f, drule::Key const & key)
{
  drule::BaseRule const * const dRule = drule::rules().Find(key);
#ifdef DEBUG
  using feature::GeomType;
  auto const geomType = f.GetGeomType();
#endif  // DEBUG
  switch (key.m_type)
  {
  case drule::symbol:
    ASSERT(dRule->GetSymbol() && !m_symbolRule &&
           (geomType == GeomType::Point || geomType == GeomType::Area),
           (m_symbolRule == nullptr, geomType, f.DebugString()));
    m_symbolRule = dRule->GetSymbol();
    break;
  case drule::caption:
    ASSERT(dRule->GetCaption() && dRule->GetCaption()->has_primary() && !m_captionRule &&
           (geomType == GeomType::Point || geomType == GeomType::Area),
           (m_captionRule == nullptr, f.DebugString()));
    m_captionRule = dRule->GetCaption();
    break;
  case drule::pathtext:
    ASSERT(dRule->GetPathtext() && dRule->GetPathtext()->has_primary() && !m_pathtextRule &&
           geomType == GeomType::Line,
           (m_pathtextRule == nullptr, geomType, f.DebugString()));
    m_pathtextRule = dRule->GetPathtext();
    break;
  case drule::shield:
    ASSERT(dRule->GetShield() && !m_shieldRule && geomType == GeomType::Line,
           (m_shieldRule == nullptr, geomType, f.DebugString()));
    m_shieldRule = dRule->GetShield();
    break;
  case drule::line:
    ASSERT(dRule->GetLine() && geomType == GeomType::Line, (geomType, f.DebugString()));
    m_lineRules.push_back(dRule->GetLine());
    break;
  case drule::area:
    ASSERT(dRule->GetArea() && geomType == GeomType::Area, (geomType, f.DebugString()));
    if (key.m_hatching)
    {
      ASSERT(!m_hatchingRule, (f.DebugString()));
      m_hatchingRule = dRule->GetArea();
    }
    else
    {
      ASSERT(!m_areaRule, (f.DebugString()));
      m_areaRule = dRule->GetArea();
    }
    break;
  // TODO(pastk) : check if circle/waymarker support exists still (not used in styles ATM).
  case drule::circle:
  case drule::waymarker:
  default:
    ASSERT(false, (key.m_type, f.DebugString()));
    return;
  }
}

Stylist::Stylist(FeatureType & f, uint8_t zoomLevel, int8_t deviceLang)
{
  feature::TypesHolder const types(f);
  Classificator const & cl = classif();

  uint32_t mainOverlayType = 0;
  if (types.Size() == 1)
    mainOverlayType = types.front();
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
      if (t == mainOverlayType ||
          (k.m_type != drule::caption && k.m_type != drule::symbol &&
           k.m_type != drule::shield && k.m_type != drule::pathtext))
      {
        if (hasHatching && k.m_type == drule::area)
          k.m_hatching = true;
        keys.push_back(k);
      }
    }
  }

  feature::FilterRulesByRuntimeSelector(f, zoomLevel, keys);

  if (keys.empty())
    return;

  // Leave only one area drule and an optional hatching drule.
  drule::MakeUnique(keys);

  for (auto const & key : keys)
    ProcessKey(f, key);

  if (m_captionRule || m_pathtextRule)
  {
    bool const auxExists = (m_captionRule && m_captionRule->has_secondary()) ||
                           (m_pathtextRule && m_pathtextRule->has_secondary());
    m_captionDescriptor.Init(f, deviceLang, zoomLevel, geomType, auxExists);

    if (m_captionDescriptor.IsHouseNumberExists())
    {
      bool isGood = true;
      if (zoomLevel < scales::GetPointHousenumbersScale())
      {
        if (geomType == feature::GeomType::Area)
        {
          // Don't display housenumbers when an area (e.g. a building) is too small.
          m2::RectD const r = f.GetLimitRect(zoomLevel);
          isGood = std::min(r.SizeX(), r.SizeY()) > scales::GetEpsilonForHousenumbers(zoomLevel);
        }
        else
          isGood = false;
      }

      if (isGood)
      {
        // Use building-address' caption drule to display house numbers.
        static auto const addressType = cl.GetTypeByPath({"building", "address"});
        if (mainOverlayType == addressType)
        {
          // Optimization: just duplicate the drule if the main type is building-address.
          ASSERT(m_captionRule, ());
          m_houseNumberRule = m_captionRule;
        }
        else
        {
          drule::KeysT addressKeys;
          cl.GetObject(addressType)->GetSuitable(zoomLevel, geomType, addressKeys);
          if (!addressKeys.empty())
          {
            // A caption drule exists for this zoom level.
            ASSERT(addressKeys.size() == 1 && addressKeys[0].m_type == drule::caption,
                   ("building-address should contain a caption drule only"));
            ASSERT(m_houseNumberRule == nullptr, ());
            m_houseNumberRule = drule::rules().Find(addressKeys[0])->GetCaption();
          }
        }
      }
    }

    if (!m_captionDescriptor.IsNameExists())
    {
      m_captionRule = nullptr;
      m_pathtextRule = nullptr;
    }
  }

  if (m_shieldRule)
  {
    m_roadShields = ftypes::GetRoadShields(f);
    if (m_roadShields.empty())
      m_shieldRule = nullptr;
  }
}

}  // namespace df
