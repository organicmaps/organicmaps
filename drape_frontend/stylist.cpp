#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
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
enum Type
{
  Line      =               1,
  Area      = Line       << 1,
  Symbol    = Area       << 1,
  Caption   = Symbol     << 1,
  Circle    = Caption    << 1,
  PathText  = Circle     << 1,
  Waymarker = PathText   << 1,
  Shield    = Waymarker  << 1,
  CountOfType = PathText + 1
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

double constexpr kMinPriority = std::numeric_limits<double>::lowest();

inline bool IsTypeOf(drule::Key const & key, int flags)
{
  int currentFlag = Line;
  while (currentFlag < CountOfType)
  {
    Type type = Type(flags & currentFlag);
    if (type != 0 && key.m_type == Convert(type))
      return true;

    currentFlag <<= 1;
  }

  return false;
}

bool IsMiddleTunnel(int const layer, double const depth)
{
  return layer != feature::LAYER_EMPTY && depth < 19000;
}

class Aggregator
{
public:
  Aggregator(FeatureType & f, feature::GeomType const type, int const zoomLevel, int const keyCount)
    : m_pointStyleFound(false)
    , m_lineStyleFound(false)
    , m_auxCaptionFound(false)
    , m_mainTextType(drule::text_type_name)
    , m_f(f)
    , m_geomType(type)
    , m_zoomLevel(zoomLevel)
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
  bool m_auxCaptionFound;
  drule::text_type_t m_mainTextType;
  buffer_vector<Stylist::TRuleWrapper, 8> m_rules;

private:
  void ProcessKey(drule::Key const & key)
  {
    double depth = key.m_priority;
    if (IsMiddleTunnel(m_depthLayer, depth) && IsTypeOf(key, Line))
    {
      double const layerPart = m_depthLayer * drule::layer_base_priority;
      double const depthPart = fmod(depth, drule::layer_base_priority);
      depth = layerPart + depthPart;
    }

    drule::BaseRule const * const dRule = drule::rules().Find(key);
    if (dRule == nullptr)
      return;

    if (dRule->GetCaption(0) != nullptr)
      m_mainTextType = dRule->GetCaptionTextType(0);

    m_auxCaptionFound |= (dRule->GetCaption(1) != nullptr);

    // Skip lines with zero width. Lines can have zero width only if they have
    // path symbols along.
    auto const lineRule = dRule->GetLine();
    if (lineRule != nullptr && (lineRule->width() < 1e-5 && !lineRule->has_pathsym()))
      return;

    m_rules.emplace_back(std::make_pair(dRule, depth));
  }

  void Init()
  {
    m_depthLayer = m_f.GetLayer();
    if (m_depthLayer == feature::LAYER_TRANSPARENT_TUNNEL)
      m_depthLayer = feature::LAYER_EMPTY;

    if (m_geomType == feature::GeomType::Point)
      m_priorityModifier = (double)m_f.GetPopulation() / 7E9;
    else
    {
      m2::RectD const r = m_f.GetLimitRect(m_zoomLevel);
      m_priorityModifier = std::min(1.0, r.SizeX() * r.SizeY() * 10000.0);
    }
  }

  FeatureType & m_f;
  feature::GeomType m_geomType;
  int const m_zoomLevel;
  double m_priorityModifier;
  int m_depthLayer;
};

uint8_t const CoastlineFlag  = 1;
uint8_t const AreaStyleFlag  = 1 << 1;
uint8_t const LineStyleFlag  = 1 << 2;
uint8_t const PointStyleFlag = 1 << 3;
}  // namespace

IsBuildingHasPartsChecker::IsBuildingHasPartsChecker()
{
  m_types.push_back(classif().GetTypeByPath({"building", "has_parts"}));
}

IsHatchingTerritoryChecker::IsHatchingTerritoryChecker()
{
  Classificator const & c = classif();
  char const * arr[][2] = {{"leisure", "nature_reserve"},
                           {"boundary", "national_park"},
                           {"landuse", "military"}};
  for (auto const & p : arr)
    m_types.push_back(c.GetTypeByPath({p[0], p[1]}));
}

void CaptionDescription::Init(FeatureType & f, int8_t deviceLang, int const zoomLevel,
                              feature::GeomType const type, drule::text_type_t const mainTextType,
                              bool const auxCaptionExists)
{
  if (auxCaptionExists || type == feature::GeomType::Line)
    f.GetPreferredNames(true /* allowTranslit */, deviceLang, m_mainText, m_auxText);
  else
    f.GetReadableName(true /* allowTranslit */, deviceLang, m_mainText);

  // Set max text size to avoid VB/IB overflow in rendering.
  size_t constexpr kMaxTextSize = 200;
  if (m_mainText.size() > kMaxTextSize)
    m_mainText = m_mainText.substr(0, kMaxTextSize) + "...";

  m_roadNumber = f.GetRoadNumber();
  m_houseNumber = f.GetHouseNumber();

  ProcessZoomLevel(zoomLevel);
  ProcessMainTextType(mainTextType);
}

std::string const & CaptionDescription::GetMainText() const
{
  return m_mainText;
}

std::string const & CaptionDescription::GetAuxText() const
{
  return m_auxText;
}

std::string const & CaptionDescription::GetRoadNumber() const
{
  return m_roadNumber;
}

bool CaptionDescription::IsNameExists() const
{
  return !m_mainText.empty() || !m_houseNumber.empty();
}

void CaptionDescription::ProcessZoomLevel(int const zoomLevel)
{
  if (zoomLevel <= scales::GetUpperWorldScale() && !m_auxText.empty())
  {
    m_auxText.clear();
  }

  if (zoomLevel < 5 && m_mainText.size() > 50)
  {
    m_mainText.clear();
    m_auxText.clear();
  }
}

void CaptionDescription::ProcessMainTextType(drule::text_type_t const & mainTextType)
{
  if (m_houseNumber.empty())
    return;

  if (mainTextType == drule::text_type_housenumber)
  {
    m_mainText.swap(m_houseNumber);
    m_houseNumber.clear();
    m_isHouseNumberInMainText = true;
  }
  else if (mainTextType == drule::text_type_name)
  {
    if (m_mainText.empty() || m_houseNumber.find(m_mainText) != std::string::npos)
    {
      m_houseNumber.swap(m_mainText);
      m_isHouseNumberInMainText = true;
    }
  }
}

Stylist::Stylist()
  : m_state(0)
{}

bool Stylist::IsCoastLine() const
{
  return (m_state & CoastlineFlag) != 0;
}

bool Stylist::AreaStyleExists() const
{
  return (m_state & AreaStyleFlag) != 0;
}

bool Stylist::LineStyleExists() const
{
  return (m_state & LineStyleFlag) != 0;
}

bool Stylist::PointStyleExists() const
{
  return (m_state & PointStyleFlag) != 0;
}

CaptionDescription const & Stylist::GetCaptionDescription() const
{
  return m_captionDescriptor;
}

void Stylist::ForEachRule(Stylist::TRuleCallback const & fn) const
{
  typedef rules_t::const_iterator const_iter;
  for (const_iter it = m_rules.begin(); it != m_rules.end(); ++it)
    fn(*it);
}

bool Stylist::IsEmpty() const
{
  return m_rules.empty();
}

void Stylist::RaiseCoastlineFlag()
{
  m_state |= CoastlineFlag;
}

void Stylist::RaiseAreaStyleFlag()
{
  m_state |= AreaStyleFlag;
}

void Stylist::RaiseLineStyleFlag()
{
  m_state |= LineStyleFlag;
}

void Stylist::RaisePointStyleFlag()
{
  m_state |= PointStyleFlag;
}

CaptionDescription & Stylist::GetCaptionDescriptionImpl()
{
  return m_captionDescriptor;
}

bool InitStylist(FeatureType & f, int8_t deviceLang, int const zoomLevel, bool buildings3d, Stylist & s)
{
  feature::TypesHolder const types(f);

  if (!buildings3d && ftypes::IsBuildingPartChecker::Instance()(types) &&
      !ftypes::IsBuildingChecker::Instance()(types))
    return false;

  drule::KeysT keys;
  auto const geomType = feature::GetDrawRule(types, zoomLevel, keys);

  feature::FilterRulesByRuntimeSelector(f, zoomLevel, keys);

  if (keys.empty())
    return false;

  drule::MakeUnique(keys);

  if (geomType.second)
    s.RaiseCoastlineFlag();

  auto const mainGeomType = feature::GeomType(geomType.first);

  switch (mainGeomType)
  {
  case feature::GeomType::Point:
    s.RaisePointStyleFlag();
    break;
  case feature::GeomType::Line :
    s.RaiseLineStyleFlag();
    break;
  case feature::GeomType::Area :
    s.RaiseAreaStyleFlag();
    break;
  default:
    ASSERT(false, ());
    return false;
  }

  Aggregator aggregator(f, mainGeomType, zoomLevel, static_cast<int>(keys.size()));
  aggregator.AggregateKeys(keys);

  CaptionDescription & descr = s.GetCaptionDescriptionImpl();
  descr.Init(f, deviceLang, zoomLevel, mainGeomType, aggregator.m_mainTextType, aggregator.m_auxCaptionFound);

  aggregator.AggregateStyleFlags(keys, descr.IsNameExists());

  if (aggregator.m_pointStyleFound)
    s.RaisePointStyleFlag();
  if (aggregator.m_lineStyleFound)
    s.RaiseLineStyleFlag();

  s.m_rules.swap(aggregator.m_rules);

  return true;
}

double GetFeaturePriority(FeatureType & f, int const zoomLevel)
{
  drule::KeysT keys;
  std::pair<int, bool> const geomType =
      feature::GetDrawRule(feature::TypesHolder(f), zoomLevel, keys);

  feature::FilterRulesByRuntimeSelector(f, zoomLevel, keys);

  auto const mainGeomType = feature::GeomType(geomType.first);

  Aggregator aggregator(f, mainGeomType, zoomLevel, static_cast<int>(keys.size()));
  aggregator.AggregateKeys(keys);

  double maxPriority = kMinPriority;
  for (auto const & rule : aggregator.m_rules)
  {
    if (rule.second > maxPriority)
      maxPriority = rule.second;
  }

  return maxPriority;
}
}  // namespace df
