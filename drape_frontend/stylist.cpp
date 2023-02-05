#include "drape_frontend/stylist.hpp"

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

float constexpr kMinPriority = std::numeric_limits<float>::lowest();

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
    if (m_featureLayer != feature::LAYER_EMPTY)
    {
      if (IsTypeOf(key, Line))
      {
        double const layerPart = m_featureLayer * drule::layer_base_priority;
        double const depthPart = fmod(depth, drule::layer_base_priority);
        depth = layerPart + depthPart;
      }
      else if (IsTypeOf(key, Area))
      {
        // Area styles have big negative priorities (like -15000), so just add layer correction.
        depth += m_featureLayer * drule::layer_base_priority;
      }
      else
      {
        /// @todo Take into account depth-layer for "point-styles". Like priority in OverlayHandle?
      }
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

    m_rules.push_back({ dRule, static_cast<float>(depth), key.m_hatching });
  }

  void Init()
  {
    m_featureLayer = m_f.GetLayer();
    // @todo m_priorityModifier is not used, try to apply to areas to render in size order.
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
  int m_featureLayer;
};
}  // namespace

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
  feature::NameParamsOut out;
  if (auxCaptionExists || type == feature::GeomType::Line)
    f.GetPreferredNames(true /* allowTranslit */, deviceLang, out);
  else
    f.GetReadableName(true /* allowTranslit */, deviceLang, out);

  m_mainText = out.GetPrimary();
  m_auxText = out.secondary;

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

CaptionDescription const & Stylist::GetCaptionDescription() const
{
  return m_captionDescriptor;
}

bool Stylist::IsEmpty() const
{
  return m_rules.empty();
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

  Classificator const & cl = classif();
  auto const & hatchingChecker = IsHatchingTerritoryChecker::Instance();
  auto const geomType = types.GetGeomType();

  drule::KeysT keys;
  size_t idx = 0;
  for (uint32_t t : types)
  {
    cl.GetObject(t)->GetSuitable(zoomLevel, geomType, keys);

    if (hatchingChecker(t))
    {
      while (idx < keys.size())
      {
        if (keys[idx].m_type == drule::area)
          keys[idx].m_hatching = true;
        ++idx;
      }
    }
    else
    {
      // GetSuitable function appends 'keys' vector, so move start index accordingly.
      idx = keys.size();
    }
  }

  feature::FilterRulesByRuntimeSelector(f, zoomLevel, keys);

  if (keys.empty())
    return false;

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

  CaptionDescription & descr = s.GetCaptionDescriptionImpl();
  descr.Init(f, deviceLang, zoomLevel, geomType, aggregator.m_mainTextType, aggregator.m_auxCaptionFound);

  aggregator.AggregateStyleFlags(keys, descr.IsNameExists());

  if (aggregator.m_pointStyleFound)
    s.m_pointStyleExists = true;
  if (aggregator.m_lineStyleFound)
    s.m_lineStyleExists = true;

  s.m_rules.swap(aggregator.m_rules);

  return true;
}

double GetFeaturePriority(FeatureType & f, int const zoomLevel)
{
  feature::TypesHolder types(f);
  drule::KeysT keys;
  feature::GetDrawRule(types, zoomLevel, keys);

  feature::FilterRulesByRuntimeSelector(f, zoomLevel, keys);

  Aggregator aggregator(f, types.GetGeomType(), zoomLevel, keys.size());
  aggregator.AggregateKeys(keys);

  float maxPriority = kMinPriority;
  for (auto const & rule : aggregator.m_rules)
  {
    if (rule.m_depth > maxPriority)
      maxPriority = rule.m_depth;
  }

  return maxPriority;
}
}  // namespace df
