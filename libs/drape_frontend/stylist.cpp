#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/cycling_infrastructure.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "drape/hatching_decl.hpp"

#include "geometry/spline.hpp"

#include <algorithm>
#include <limits>

namespace df
{
m2::SharedSpline Stylist::SideRules::GetArrowSpline(m2::SharedSpline const & piece) const
{
  if (!m_reverseArrow)
    return piece;
  auto path = piece->GetPath();
  std::reverse(path.begin(), path.end());
  return m2::SharedSpline(std::move(path));
}

IsHatchingTerritoryChecker::TwoLevel45::TwoLevel45()
  : ftypes::BaseCheckerEx({{"amenity", "prison"},
                           {"boundary", "aboriginal_lands"},
                           {"boundary", "national_park"},
                           {"landuse", "military"},
                           {"leisure", "nature_reserve"}})
{}

IsHatchingTerritoryChecker::IsHatchingTerritoryChecker()
{
  Classificator const & c = classif();
  m_3level45 = c.GetTypeByPath({"boundary", "protected_area", "1"});
  m_2levelDash = c.GetTypeByPath({"natural", "wetland"});
}

std::string_view IsHatchingTerritoryChecker::GetHatch(uint32_t type) const
{
  // Matching with subtypes (see Stylist_IsHatching test).

  if (m_3level45 == ftype::Trunc(type, 3))
    return dp::k45dHatching;
  if (m_2level45(type))
    return dp::k45dHatching;

  if (m_2levelDash == ftype::Trunc(type, 2))
    return dp::kDashHatching;

  return {};
}

std::string_view IsHatchingTerritoryChecker::GetHatch(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    auto s = GetHatch(t);
    if (!s.empty())
      return s;
  }
  return {};
}

IsAreaPatternChecker::Stipple::Stipple()
  : ftypes::BaseCheckerEx({{"natural", "beach"}, {"natural", "desert"}})  // natural=sand is a beach subtype
{}

IsAreaPatternChecker::Speckle::Speckle() : ftypes::BaseCheckerEx({{"natural", "scree"}, {"natural", "bare_rock"}}) {}

IsAreaPatternChecker::Grid::Grid() : ftypes::BaseCheckerEx({{"landuse", "orchard"}, {"landuse", "vineyard"}}) {}

std::string_view IsAreaPatternChecker::GetPattern(uint32_t type) const
{
  if (m_stipple(type))
    return dp::kStipplePattern;
  if (m_speckle(type))
    return dp::kSpecklePattern;
  if (m_grid(type))
    return dp::kGridPattern;
  return {};
}

std::string_view IsAreaPatternChecker::GetPattern(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    auto s = GetPattern(t);
    if (!s.empty())
      return s;
  }
  return {};
}

void CaptionDescription::Init(FeatureType & f, int8_t deviceLang, int zoomLevel, feature::GeomType geomType,
                              bool auxCaptionExists)
{
  if (auto const & info = f.GetID().m_mwmId.GetInfo())
    m_mwmRegionLang = feature::GetRegionLang(info->GetRegionData());

  // An unqualified OSM `name=` is reported as kDefaultCode, which is not a real BCP-47 tag.
  // Resolve it to the region's on-the-ground language so HarfBuzz applies the matching OpenType
  // `locl` glyph variants (Turkish dotless-i, Serbian Cyrillic, CJK regional forms). Multi-lingual
  // regions may guess wrong, but a shared-script mismatch is mostly harmless and strictly better
  // than passing no hint at all.
  auto const localizeLang = [this](int8_t lang)
  { return lang == StringUtf8Multilang::kDefaultCode ? m_mwmRegionLang : lang; };

  feature::NameParamsOut out;
  // TODO(pastk) : remove forced secondary text for all lines and set it via styles for major roads and rivers only.
  // ATM even minor paths/streams/etc use secondary which makes their pathtexts take much more space.
  if (zoomLevel > scales::GetUpperWorldScale() && (auxCaptionExists || geomType == feature::GeomType::Line))
  {
    // Get both primary and secondary/aux names.
    f.GetPreferredNames(true /* allowTranslit */, deviceLang, out);
    m_auxText = out.secondary;
    m_auxTextLang = localizeLang(out.secondaryLang);
  }
  else
  {
    // Returns primary name only.
    f.GetReadableName(true /* allowTranslit */, deviceLang, out);
  }
  m_mainText = out.GetPrimary();
  m_mainTextLang = localizeLang(out.primaryLang);
  ASSERT(m_auxText.empty() || !m_mainText.empty(), ("auxText without mainText"));

  uint8_t constexpr kLongCaptionsMaxZoom = 4;
  size_t constexpr kLowWorldMaxTextSize = 50;
  if (zoomLevel <= kLongCaptionsMaxZoom && m_mainText.size() > kLowWorldMaxTextSize)
  {
    m_mainText.clear();
    m_auxText.clear();
    m_mainTextLang = m_auxTextLang = StringUtf8Multilang::kUnsupportedLanguageCode;
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
    // TODO(pastk) : its not obvious that a housenumber display is dependent on a secondary caption drule existance in
    // styles.
    m_houseNumberText = f.GetHouseNumber();
    if (!m_houseNumberText.empty() && !m_mainText.empty() && m_houseNumberText.find(m_mainText) != std::string::npos)
    {
      m_mainText.clear();
      m_mainTextLang = StringUtf8Multilang::kUnsupportedLanguageCode;
    }
  }
}

void Stylist::ProcessKey(FeatureType & f, drule::Key const & key)
{
  drule::BaseRule const * const dRule = m_rulesHolder.Find(key);
#ifdef DEBUG
  using feature::GeomType;
  auto const geomType = f.GetGeomType();
#endif  // DEBUG
  switch (key.m_type)
  {
  case drule::symbol:
    ASSERT(dRule->GetSymbol() && !m_symbolRule && (geomType == GeomType::Point || geomType == GeomType::Area),
           (m_symbolRule == nullptr, geomType, f.DebugString()));
    m_symbolRule = dRule->GetSymbol();
    break;
  case drule::caption:
    ASSERT(dRule->GetCaption() && dRule->GetCaption()->primary.has_value() && !m_captionRule &&
               (geomType == GeomType::Point || geomType == GeomType::Area),
           (m_captionRule == nullptr, f.DebugString()));
    m_captionRule = dRule->GetCaption();
    break;
  case drule::pathtext:
    ASSERT(dRule->GetPathtext() && dRule->GetPathtext()->primary.has_value() && !m_pathtextRule &&
               geomType == GeomType::Line,
           (m_pathtextRule == nullptr, geomType, f.DebugString()));
    m_pathtextRule = dRule->GetPathtext();
    break;
  case drule::shield:
    ASSERT(dRule->GetShield() && !m_shieldRule && geomType == GeomType::Line,
           (m_shieldRule == nullptr, geomType, f.DebugString()));
    ASSERT_GREATER(dRule->GetShield()->height, 0, (f.DebugString()));
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
  default: ASSERT(false, (key.m_type, f.DebugString())); return;
  }
}

Stylist::Stylist(FeatureType & f, uint8_t zoomLevel, int8_t deviceLang, MapStyle style)
  : m_rulesHolder(drule::GetRules(style))
{
#ifdef DEBUG
  ASSERT(classificator::IsStyleLoaded(style), ("Drawing rules for the style are not loaded", style));
#endif  // DEBUG

  feature::TypesHolder const types(f);
  Classificator const & cl = classif(style);

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
  auto const sideData = geomType == feature::GeomType::Line ? cycling::Decode(f.GetCyclingInfrastructure())
                                                            : std::optional<cycling::Sides>{};
  bool const useSideLines = sideData && zoomLevel >= kCyclewaySideMinZoom;
  MapStyle const cycleStyle = MapStyleIsDark(style) ? MapStyleCyclingDark : MapStyleCyclingLight;
  auto const & cycleClassif = classif(cycleStyle);
  auto const & cycleRules = drule::GetRules(cycleStyle);
  auto const normalStyle = MapStyleIsDark(style) ? MapStyleDefaultDark : MapStyleDefaultLight;
  auto const & normalClassif = classif(normalStyle);
  auto const & normalRules = drule::GetRules(normalStyle);
  auto const indicator = [&](drule::LineRule const * rule)
  {
    m_cyclewayIndicatorRules.push_back(*rule);
    auto & copy = m_cyclewayIndicatorRules.back();
    if (copy.dashdot && copy.width > 0)
    {
      auto const scale = CyclewayIndicatorWidth(zoomLevel) / copy.width;
      for (auto & length : copy.dashdot->dd)
        length *= scale;
      copy.dashdot->offset *= scale;
    }
    copy.width = CyclewayIndicatorWidth(zoomLevel);
    return &copy;
  };
  auto const highwayRoot = cl.GetTypeByPath({"highway"});
  auto const cyclewayRoot = cl.GetTypeByPath({"cyclewaytag"});
  auto const dedicatedCycleway = cl.GetTypeByPath({"highway", "cycleway"});
  auto const bicycleFootway = cl.GetTypeByPathSafe({"highway", "footway", "bicycle"});
  auto const bicyclePath = cl.GetTypeByPathSafe({"highway", "path", "bicycle"});
  auto const isDedicated = [&](uint32_t type)
  {
    return ftype::Trunc(type, 2) == dedicatedCycleway ||
           (bicycleFootway != Classificator::INVALID_TYPE && ftype::Trunc(type, 3) == bicycleFootway) ||
           (bicyclePath != Classificator::INVALID_TYPE && ftype::Trunc(type, 3) == bicyclePath);
  };
  bool const restoreCore = geomType == feature::GeomType::Line && style != cycleStyle &&
                           std::any_of(types.begin(), types.end(), isDedicated);
  drule::KeysT cycleCoreKeys;
  auto const collectCore = [&](uint32_t type)
  {
    drule::KeysT cyclingKeys;
    cycleClassif.GetObject(type)->GetSuitable(zoomLevel, geomType, cyclingKeys);
    for (auto const & key : cyclingKeys)
      if (key.m_type == drule::line)
        cycleCoreKeys.push_back(key);
  };
  if (sideData && !useSideLines &&
      std::none_of(types.begin(), types.end(), [&](uint32_t type) { return ftype::Trunc(type, 1) == cyclewayRoot; }))
  {
    // Side metadata survives the classifier type cap. Do not lose overview
    // styling merely because the aggregate cycleway type was dropped.
    auto summary = cycling::Kind::None;
    for (auto const kind : {sideData->m_left.m_kind, sideData->m_right.m_kind})
      if (kind == cycling::Kind::Shared)
        summary = kind;
      else if (kind == cycling::Kind::Lane && summary != cycling::Kind::Shared)
        summary = kind;
      else if (kind == cycling::Kind::Track && summary == cycling::Kind::None)
        summary = kind;
    if (summary != cycling::Kind::None)
      collectCore(cycleClassif.GetTypeByPath({"cyclewaytag", cycling::TypeName(summary)}));
  }
  if (useSideLines)
  {
    auto const hasType = [&](char const * name)
    {
      auto const type = cl.GetTypeByPath({"hwtag", name});
      return std::find(types.begin(), types.end(), type) != types.end();
    };
    bool const roadOneWayForBikes = hasType("onedir_bicycle") || (hasType("oneway") && !hasType("bidir_bicycle"));
    auto const resolveDirection = [&](cycling::Direction direction)
    {
      return direction == cycling::Direction::Unknown && roadOneWayForBikes ? cycling::Direction::Forward : direction;
    };
    drule::KeysT arrowKeys;
    cycleClassif.GetObject(cycleClassif.GetTypeByPath({"hwtag", "onedir_bicycle"}))
        ->GetSuitable(zoomLevel, geomType, arrowKeys);
    auto const collect = [&](cycling::Side const & side, int sign)
    {
      if (side.m_kind == cycling::Kind::None)
        return;
      auto const type = cycleClassif.GetTypeByPath({"cyclewaytag", cycling::TypeName(side.m_kind)});
      drule::KeysT sideKeys;
      cycleClassif.GetObject(type)->GetSuitable(zoomLevel, geomType, sideKeys);
      SideRules rules{sign, {}};
      if (sign != 0)
        rules.m_junctions = side.m_junctions;
      drule::LineRule const * stroke = nullptr;
      for (auto const & key : sideKeys)
      {
        if (key.m_type != drule::line)
          continue;
        auto const rule = cycleRules.Find(key)->GetLine();
        if (rule && !rule->pathsym && (!stroke || rule->priority > stroke->priority))
          stroke = rule;
      }
      if (stroke)
        rules.m_rules.push_back(indicator(stroke));
      auto const direction = resolveDirection(side.m_direction);
      if (stroke && (direction == cycling::Direction::Forward || direction == cycling::Direction::Backward))
      {
        // Side metadata is authoritative even when the aggregate road type is bidirectional.
        // Reuse the existing zoom-dependent symbol for this side's own pattern.
        std::string const prefix =
            side.m_kind == cycling::Kind::Track ? "cycling-arrow-solid-" : "cycling-arrow-patterned-";
        for (auto const & key : arrowKeys)
        {
          if (key.m_type != drule::line)
            continue;
          auto const rule = cycleRules.Find(key)->GetLine();
          if (rule->pathsym && rule->pathsym->name.starts_with(prefix))
          {
            rules.m_arrowRule = rule;
            rules.m_reverseArrow = direction == cycling::Direction::Backward;
            break;
          }
        }
      }
      if (!rules.m_rules.empty())
        m_cyclewaySides.push_back(std::move(rules));
    };
    if (sideData->m_left.m_kind != cycling::Kind::Shared)
      collect(sideData->m_left, -1);
    if (sideData->m_right.m_kind != cycling::Kind::Shared)
      collect(sideData->m_right, 1);

    std::optional<cycling::Side> shared;
    for (auto const & side : {sideData->m_left, sideData->m_right})
    {
      if (side.m_kind != cycling::Kind::Shared)
        continue;
      auto const direction = resolveDirection(side.m_direction);
      if (!shared)
        shared = cycling::Side{cycling::Kind::Shared, direction};
      else if (shared->m_direction != direction)
        shared->m_direction = cycling::Direction::Both;
    }
    // A single centerline summarizes shared sides. Only unanimous one-way
    // directions can safely supply an arrow for that combined treatment.
    if (shared)
      collect(*shared, 0);
  }

  drule::KeysT keys;
  LineRulesT normalRoadLines;
  float cyclewayRoadWidth = 0;
  for (uint32_t t : types)
  {
    if (useSideLines && ftype::Trunc(t, 1) == cyclewayRoot)
      continue;  // Draw attached facilities beside the primary road, not over it.
    drule::KeysT typeKeys;
    cl.GetObject(t)->GetSuitable(zoomLevel, geomType, typeKeys);
    if (useSideLines && ftype::Trunc(t, 1) == highwayRoot)
    {
      drule::KeysT normalKeys;
      normalClassif.GetObject(t)->GetSuitable(zoomLevel, geomType, normalKeys);
      feature::FilterRulesByRuntimeSelector(f, zoomLevel, normalRules, normalKeys);
      for (auto const & key : normalKeys)
        if (key.m_type == drule::line && !normalRules.Find(key)->GetLine()->pathsym)
          normalRoadLines.push_back(normalRules.Find(key)->GetLine());

      auto widthKeys = normalKeys;
      auto const structure = ftype::GetLevel(t) >= 3 ? normalClassif.GetObject(ftype::Trunc(t, 3))->GetName() : "";
      if (structure == "bridge" || structure == "tunnel")
      {
        // Only dimension the cycling displacement from the ordinary road.
        // The complete structural rules above remain part of the rendered road.
        widthKeys.clear();
        normalClassif.GetObject(ftype::Trunc(t, 2))->GetSuitable(zoomLevel, geomType, widthKeys);
        feature::FilterRulesByRuntimeSelector(f, zoomLevel, normalRules, widthKeys);
      }
      for (auto const & key : widthKeys)
      {
        if (key.m_type != drule::line)
          continue;
        auto const rule = normalRules.Find(key)->GetLine();
        if (!rule->pathsym)
          cyclewayRoadWidth = std::max(cyclewayRoadWidth, rule->width);
      }
      typeKeys.erase(
          std::remove_if(typeKeys.begin(), typeKeys.end(), [](auto const & key) { return key.m_type == drule::line; }),
          typeKeys.end());
    }
    bool const hasHatching = hatchingChecker(t);
    bool const restoreSummary = geomType == feature::GeomType::Line && style != cycleStyle && !useSideLines &&
                                ftype::Trunc(t, 1) == cyclewayRoot;
    if ((restoreCore && isDedicated(t)) || restoreSummary)
      collectCore(t);

    for (auto & k : typeKeys)
    {
      if (k.m_type == drule::line && ((restoreCore && ftype::Trunc(t, 1) == highwayRoot) || restoreSummary))
        continue;
      // Take overlay drules from the main type only.
      if (t == mainOverlayType || (k.m_type != drule::caption && k.m_type != drule::symbol &&
                                   k.m_type != drule::shield && k.m_type != drule::pathtext))
      {
        if (hasHatching && k.m_type == drule::area)
          k.m_hatching = true;
        keys.push_back(k);
      }
    }
  }

  for (auto & side : m_cyclewaySides)
    side.m_roadWidth = cyclewayRoadWidth;

  feature::FilterRulesByRuntimeSelector(f, zoomLevel, m_rulesHolder, keys);
  feature::FilterRulesByRuntimeSelector(f, zoomLevel, cycleRules, cycleCoreKeys);

  // Cycling direction symbols belong to hwtag types, not the facility stroke.
  // Borrow them in generic modes too, retaining the original runtime selectors
  // (notably the bicycle contraflow exception to a car one-way restriction).
  if (geomType == feature::GeomType::Line && style != cycleStyle && !useSideLines)
  {
    drule::KeysT arrows;
    for (uint32_t type : types)
    {
      drule::KeysT candidates;
      cycleClassif.GetObject(type)->GetSuitable(zoomLevel, geomType, candidates);
      for (auto const & key : candidates)
      {
        if (key.m_type != drule::line)
          continue;
        auto const rule = cycleRules.Find(key)->GetLine();
        if (rule && rule->pathsym && rule->pathsym->name.find("cycling-arrow-") == 0)
          arrows.push_back(key);
      }
    }
    feature::FilterRulesByRuntimeSelector(f, zoomLevel, cycleRules, arrows);
    if (!arrows.empty())
    {
      keys.erase(std::remove_if(keys.begin(), keys.end(), [&](auto const & key)
      { return key.m_type == drule::line && m_rulesHolder.Find(key)->GetLine()->pathsym.has_value(); }),
                 keys.end());
      cycleCoreKeys.insert(cycleCoreKeys.end(), arrows.begin(), arrows.end());
    }
  }

  if (useSideLines)
  {
    auto const isBikeArrow = [](drule::LineRule const * rule)
    { return rule->pathsym && rule->pathsym->name.starts_with("cycling-arrow-"); };
    keys.erase(std::remove_if(keys.begin(), keys.end(), [&](auto const & key)
    { return key.m_type == drule::line && isBikeArrow(m_rulesHolder.Find(key)->GetLine()); }),
               keys.end());
    cycleCoreKeys.erase(std::remove_if(cycleCoreKeys.begin(), cycleCoreKeys.end(), [&](auto const & key)
    { return key.m_type == drule::line && isBikeArrow(cycleRules.Find(key)->GetLine()); }),
                        cycleCoreKeys.end());
  }

  if (keys.empty() && cycleCoreKeys.empty() && normalRoadLines.empty() && m_cyclewaySides.empty())
    return;

  // Leave only one area drule and an optional hatching drule.
  drule::MakeUnique(keys);

  for (auto const & key : keys)
    ProcessKey(f, key);
  m_lineRules.insert(m_lineRules.end(), normalRoadLines.begin(), normalRoadLines.end());
  drule::MakeUnique(cycleCoreKeys);
  for (auto const & key : cycleCoreKeys)
    m_lineRules.push_back(cycleRules.Find(key)->GetLine());

  if (!useSideLines && std::any_of(types.begin(), types.end(), [&](uint32_t type)
  { return isDedicated(type) || ftype::Trunc(type, 1) == cyclewayRoot; }))
  {
    drule::LineRule const * stroke = nullptr;
    for (auto const type : types)
    {
      if (!isDedicated(type) && ftype::Trunc(type, 1) != cyclewayRoot)
        continue;
      drule::KeysT candidates;
      cycleClassif.GetObject(type)->GetSuitable(zoomLevel, geomType, candidates);
      feature::FilterRulesByRuntimeSelector(f, zoomLevel, cycleRules, candidates);
      for (auto const & key : candidates)
      {
        if (key.m_type != drule::line)
          continue;
        auto const rule = cycleRules.Find(key)->GetLine();
        if (!rule->pathsym && rule->width > 0 && (!stroke || rule->priority > stroke->priority))
          stroke = rule;
      }
    }
    if (stroke)
    {
      auto const normalized = indicator(stroke);
      m_lineRules.erase(
          std::remove_if(m_lineRules.begin(), m_lineRules.end(), [](auto const rule) { return !rule->pathsym; }),
          m_lineRules.end());
      m_lineRules.push_back(normalized);
    }
  }

  if (m_captionRule || m_pathtextRule)
  {
    bool const auxExists = (m_captionRule && m_captionRule->secondary.has_value()) ||
                           (m_pathtextRule && m_pathtextRule->secondary.has_value());
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
            m_houseNumberRule = m_rulesHolder.Find(addressKeys[0])->GetCaption();
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
