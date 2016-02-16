#include "drape_frontend/stylist.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/scales.hpp"

#include "std/limits.hpp"

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

double constexpr kMinPriority = numeric_limits<double>::lowest();

// ==================================== //

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

class KeyFunctor
{
public:
  KeyFunctor(FeatureType const & f,
             feature::EGeomType type,
             int const zoomLevel,
             int const keyCount,
             CaptionDescription & descr)
    : m_pointStyleFound(false)
    , m_lineStyleFound(false)
    , m_iconFound(false)
    , m_captionWithoutOffsetFound(false)
    , m_auxCaptionFound(false)
    , m_mainTextType(drule::text_type_name)
    , m_descrInit(false)
    , m_f(f)
    , m_geomType(type)
    , m_zoomLevel(zoomLevel)
    , m_descr(descr)
  {
    m_rules.reserve(keyCount);
    Init();
  }

  void ProcessKey(drule::Key const & key)
  {
    drule::BaseRule const * const dRule = drule::rules().Find(key);

    if (!dRule->TestFeature(m_f, m_zoomLevel))
      return;

    double depth = key.m_priority;
    if (IsMiddleTunnel(m_depthLayer, depth) &&
        IsTypeOf(key, Line | Area | Waymarker))
    {
      double const layerPart = m_depthLayer * drule::layer_base_priority;
      double const depthPart = fmod(depth, drule::layer_base_priority);
      depth = layerPart + depthPart;
    }

    if (IsTypeOf(key, Caption | Symbol | Circle | PathText))
    {
      depth += m_priorityModifier;
      if (m_geomType == feature::GEOM_POINT) ++depth;
    }
    else if (IsTypeOf(key, Area))
      depth -= m_priorityModifier;

    if (dRule->GetCaption(0) != nullptr)
    {
      InitCaptionDescription();
      m_mainTextType = dRule->GetCaptionTextType(0);
    }

    bool const isNonEmptyCaption = IsTypeOf(key, Caption) && IsNameExists();
    m_pointStyleFound |= (IsTypeOf(key, Symbol | Circle) || isNonEmptyCaption);
    m_lineStyleFound  |= IsTypeOf(key, Line);
    m_auxCaptionFound |= (dRule->GetCaption(1) != nullptr);

    m_rules.push_back(make_pair(dRule, depth));
  }

  bool m_pointStyleFound;
  bool m_lineStyleFound;
  bool m_iconFound;
  bool m_captionWithoutOffsetFound;
  bool m_auxCaptionFound;
  buffer_vector<Stylist::TRuleWrapper, 8> m_rules;
  drule::text_type_t m_mainTextType;
  bool m_descrInit;

private:
  void Init()
  {
    m_depthLayer = m_f.GetLayer();
    if (m_depthLayer == feature::LAYER_TRANSPARENT_TUNNEL)
      m_depthLayer = feature::LAYER_EMPTY;

    if (m_geomType == feature::GEOM_POINT)
      m_priorityModifier = (double)m_f.GetPopulation() / 7E9;
    else
    {
      m2::RectD const r = m_f.GetLimitRect(m_zoomLevel);
      m_priorityModifier = min(1.0, r.SizeX() * r.SizeY() * 10000.0);
    }
  }

  void InitCaptionDescription()
  {
    if (!m_descrInit)
    {
      m_descr.Init(m_f, m_zoomLevel);
      m_descrInit = true;
    }
  }

  inline bool IsNameExists() const
  {
    ASSERT(m_descrInit, ());
    return m_descr.IsNameExists();
  }

private:
  FeatureType const & m_f;
  feature::EGeomType m_geomType;
  int const m_zoomLevel;
  double m_priorityModifier;
  int m_depthLayer;
  CaptionDescription & m_descr;
};

const uint8_t CoastlineFlag  = 1;
const uint8_t AreaStyleFlag  = 1 << 1;
const uint8_t LineStyleFlag  = 1 << 2;
const uint8_t PointStyleFlag = 1 << 3;

} // namespace

// ==================================== //

void CaptionDescription::Init(FeatureType const & f,
                              int const zoomLevel)
{
  f.GetPreferredNames(m_mainText, m_auxText);

  m_roadNumber = f.GetRoadNumber();
  m_houseNumber = f.GetHouseNumber();

  // Mark houses without names/numbers so user can select them by single tap.
  if (m_houseNumber.empty() && m_mainText.empty() && ftypes::IsBuildingChecker::Instance()(f))
    m_houseNumber = "·";

  SwapCaptions(zoomLevel);
  DiscardLongCaption(zoomLevel);
}

void CaptionDescription::FormatCaptions(FeatureType const & f,
                                        feature::EGeomType type,
                                        drule::text_type_t mainTextType,
                                        bool auxCaptionExists)
{
  if (!auxCaptionExists && !m_auxText.empty() && type != feature::GEOM_LINE)
  {
    f.GetReadableName(m_mainText);
    if (m_mainText == m_auxText)
      m_auxText.clear();
  }

  if (mainTextType == drule::text_type_housenumber)
  {
    m_mainText = move(m_houseNumber);
  }
  else if (mainTextType == drule::text_type_name)
  {
    if (!m_houseNumber.empty())
    {
      if (m_mainText.empty() || m_houseNumber.find(m_mainText) != string::npos)
        m_houseNumber.swap(m_mainText);
    }
  }
}

string const & CaptionDescription::GetMainText() const
{
  return m_mainText;
}

string const & CaptionDescription::GetAuxText() const
{
  return m_auxText;
}

string const & CaptionDescription::GetRoadNumber() const
{
  return m_roadNumber;
}

string CaptionDescription::GetPathName() const
{
  // Always concat names for linear features because we process only one draw rule now.
  if (m_mainText.empty())
    return m_mainText;
  else
    return m_mainText + "   " + m_auxText;
}

bool CaptionDescription::IsNameExists() const
{
  return !m_mainText.empty() || !m_houseNumber.empty();
}

void CaptionDescription::SwapCaptions(int const zoomLevel)
{
  if (zoomLevel <= scales::GetUpperWorldScale() && !m_auxText.empty())
  {
    m_mainText.swap(m_auxText);
    m_auxText.clear();
  }
}

void CaptionDescription::DiscardLongCaption(int const zoomLevel)
{
  if (zoomLevel < 5 && m_mainText.size() > 50)
    m_mainText.clear();
}

// ==================================== //

Stylist::Stylist()
  : m_state(0)
{
}

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

bool InitStylist(FeatureType const & f, int const zoomLevel, bool buildings3d, Stylist & s)
{
  feature::TypesHolder const types(f);

  if (!buildings3d && ftypes::IsBuildingPartChecker::Instance()(types) &&
      !ftypes::IsBuildingChecker::Instance()(types))
    return false;

  drule::KeysT keys;
  pair<int, bool> const geomType = feature::GetDrawRule(types, zoomLevel, keys);

  if (keys.empty())
    return false;

  drule::MakeUnique(keys);

  if (geomType.second)
    s.RaiseCoastlineFlag();

  feature::EGeomType mainGeomType = feature::EGeomType(geomType.first);

  switch (mainGeomType)
  {
  case feature::GEOM_POINT:
    s.RaisePointStyleFlag();
    break;
  case feature::GEOM_LINE :
    s.RaiseLineStyleFlag();
    break;
  case feature::GEOM_AREA :
    s.RaiseAreaStyleFlag();
    break;
  default:
    ASSERT(false, ());
    return false;
  }

  CaptionDescription & descr = s.GetCaptionDescriptionImpl();

  KeyFunctor keyFunctor(f, mainGeomType, zoomLevel, keys.size(), descr);
  for (auto const & key : keys)
    keyFunctor.ProcessKey(key);

  if (keyFunctor.m_rules.empty())
    return false;

  if (keyFunctor.m_pointStyleFound)
    s.RaisePointStyleFlag();
  if (keyFunctor.m_lineStyleFound)
    s.RaiseLineStyleFlag();

  s.m_rules.swap(keyFunctor.m_rules);

  if (keyFunctor.m_descrInit)
    descr.FormatCaptions(f, mainGeomType, keyFunctor.m_mainTextType, keyFunctor.m_auxCaptionFound);

  return true;
}

double GetFeaturePriority(FeatureType const & f, int const zoomLevel)
{
  drule::KeysT keys;
  pair<int, bool> const geomType = feature::GetDrawRule(f, zoomLevel, keys);

  feature::EGeomType const mainGeomType = feature::EGeomType(geomType.first);

  CaptionDescription descr;

  KeyFunctor keyFunctor(f, mainGeomType, zoomLevel, keys.size(), descr);
  for (auto const & key : keys)
    keyFunctor.ProcessKey(key);

  double maxPriority = kMinPriority;
  for (auto const & rule : keyFunctor.m_rules)
  {
    if (rule.second > maxPriority)
      maxPriority = rule.second;
  }

  return maxPriority;
}

} // namespace df
