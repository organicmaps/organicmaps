#include "feature_data.hpp"
#include "feature_impl.hpp"
#include "classificator.hpp"
#include "feature.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"


using namespace feature;

////////////////////////////////////////////////////////////////////////////////////
// TypesHolder implementation
////////////////////////////////////////////////////////////////////////////////////

TypesHolder::TypesHolder(FeatureBase const & f)
: m_size(0), m_geoType(f.GetFeatureType())
{
  f.ForEachTypeRef(*this);
}

string TypesHolder::DebugPrint() const
{
  Classificator const & c = classif();

  string s;
  for (size_t i = 0; i < Size(); ++i)
    s += c.GetFullObjectName(m_types[i]) + "  ";
  return s;
}

void TypesHolder::Remove(uint32_t t)
{
  if (m_size > 0)
  {
    uint32_t * e = m_types + m_size;
    if (std::remove(m_types, e, t) != e)
      --m_size;
  }
}

namespace
{

class UselessTypesChecker
{
  vector<uint32_t> m_types;
public:
  UselessTypesChecker()
  {
    Classificator const & c = classif();

    char const * arr1[][1] = { { "building" }, { "oneway" }, { "lit" } };

    for (size_t i = 0; i < ARRAY_SIZE(arr1); ++i)
      m_types.push_back(c.GetTypeByPath(vector<string>(arr1[i], arr1[i] + 1)));
  }
  bool operator() (uint32_t t) const
  {
    return (find(m_types.begin(), m_types.end(), t) != m_types.end());
  }
};

}

void TypesHolder::SortBySpec()
{
  if (m_size < 2)
    return;

  // do very simple thing - put "very common" types to the end
  static UselessTypesChecker checker;
  (void) RemoveIfKeepValid(m_types, m_types + m_size, bind<bool>(cref(checker), _1));
}

////////////////////////////////////////////////////////////////////////////////////
// FeatureParamsBase implementation
////////////////////////////////////////////////////////////////////////////////////

void FeatureParamsBase::MakeZero()
{
  layer = 0;
  rank = 0;
  ref.clear();
  house.Clear();
  name.Clear();
}

bool FeatureParamsBase::operator == (FeatureParamsBase const & rhs) const
{
  return (name == rhs.name && house == rhs.house && ref == rhs.ref &&
          layer == rhs.layer && rank == rhs.rank);
}

bool FeatureParamsBase::CheckValid() const
{
   CHECK(layer > LAYER_LOW && layer < LAYER_HIGH, ());
   return true;
}

string FeatureParamsBase::DebugString() const
{
  string utf8name;
  name.GetString(0, utf8name);

  return ("'" + utf8name + "' Layer:" + DebugPrint(layer) +
          (rank != 0 ? " Rank:" + DebugPrint(rank) : "") +
          (!house.IsEmpty() ? " House:" + house.Get() : "") +
          (!ref.empty() ? " Ref:" + ref : "") + " ");
}

namespace
{

// Most used dummy values are taken from
// http://taginfo.openstreetmap.org/keys/addr%3Ahousename#values
bool IsDummyName(string const & s)
{
  return (s.empty() ||
          s == "Bloc" || s == "bloc" ||
          s == "жилой дом" ||
          s == "Edificio" || s == "edificio");
}

struct IsBadChar
{
  bool operator() (char c) const { return (c == '\n'); }
};

}

bool FeatureParams::AddName(string const & lang, string const & s)
{
  if (IsDummyName(s))
    return false;

  name.AddString(lang, s);
  return true;
}

bool FeatureParams::AddHouseName(string const & s)
{
  if (IsDummyName(s) || name.FindString(s) != StringUtf8Multilang::UNSUPPORTED_LANGUAGE_CODE)
    return false;

  // Most names are house numbers by statistics.
  if (house.IsEmpty() && AddHouseNumber(s))
    return true;

  // Add as a default name if we don't have it yet.
  string dummy;
  if (!name.GetString(StringUtf8Multilang::DEFAULT_CODE, dummy))
  {
    name.AddString(StringUtf8Multilang::DEFAULT_CODE, s);
    return true;
  }

  return false;
}

bool FeatureParams::AddHouseNumber(string const & ss)
{
  if (!feature::IsHouseNumber(ss))
    return false;

  // Remove trailing zero's from house numbers.
  // It's important for debug checks of serialized-deserialized feature.
  string s(ss);
  uint64_t n;
  if (strings::to_uint64(s, n))
    s = strings::to_string(n);

  house.Set(s);
  return true;
}

void FeatureParams::AddStreetAddress(string const & s)
{
  m_street = s;

  // Erase bad chars (\n) because we write addresses to txt file.
  m_street.erase(remove_if(m_street.begin(), m_street.end(), IsBadChar()), m_street.end());

  // Osm likes to put house numbers into addr:street field.
  size_t i = m_street.find_last_of("\t ");
  if (i != string::npos)
  {
    ++i;
    uint64_t n;
    if (strings::to_uint64(m_street.substr(i), n))
      m_street.erase(i);
  }
}

bool FeatureParams::FormatFullAddress(m2::PointD const & pt, string & res) const
{
  if (!m_street.empty() && !house.IsEmpty())
  {
    res = m_street + "|" + house.Get() + "|"
        + strings::to_string(MercatorBounds::YToLat(pt.y)) + "|"
        + strings::to_string(MercatorBounds::XToLon(pt.x)) + '\n';
    return true;
  }

  return false;
}

void FeatureParams::SetGeomType(feature::EGeomType t)
{
  switch (t)
  {
  case GEOM_POINT: m_geomType = HEADER_GEOM_POINT; break;
  case GEOM_LINE: m_geomType = HEADER_GEOM_LINE; break;
  case GEOM_AREA: m_geomType = HEADER_GEOM_AREA; break;
  default: ASSERT(false, ());
  }
}

void FeatureParams::SetGeomTypePointEx()
{
  ASSERT(m_geomType == HEADER_GEOM_POINT || m_geomType == HEADER_GEOM_POINT_EX, ());
  ASSERT(!house.IsEmpty(), ());

  m_geomType = HEADER_GEOM_POINT_EX;
}

feature::EGeomType FeatureParams::GetGeomType() const
{
  CHECK_NOT_EQUAL(m_geomType, 0xFF, ());
  switch (m_geomType)
  {
  case HEADER_GEOM_LINE: return GEOM_LINE;
  case HEADER_GEOM_AREA: return GEOM_AREA;
  default: return GEOM_POINT;
  }
}

uint8_t FeatureParams::GetTypeMask() const
{
  CHECK_NOT_EQUAL(m_geomType, 0xFF, ());
  return m_geomType;
}

void FeatureParams::AddTypes(FeatureParams const & rhs, uint32_t skipType2)
{
  if (skipType2 == 0)
  {
    m_Types.insert(m_Types.end(), rhs.m_Types.begin(), rhs.m_Types.end());
  }
  else
  {
    for (size_t i = 0; i < rhs.m_Types.size(); ++i)
    {
      uint32_t t = rhs.m_Types[i];
      ftype::TruncValue(t, 2);
      if (t != skipType2)
        m_Types.push_back(rhs.m_Types[i]);
    }
  }
}

bool FeatureParams::FinishAddingTypes()
{
  sort(m_Types.begin(), m_Types.end());
  m_Types.erase(unique(m_Types.begin(), m_Types.end()), m_Types.end());

  if (m_Types.size() > max_types_count)
    m_Types.resize(max_types_count);

  return !m_Types.empty();
}

void FeatureParams::SetType(uint32_t t)
{
  m_Types.clear();
  m_Types.push_back(t);
}

bool FeatureParams::PopAnyType(uint32_t & t)
{
  CHECK(!m_Types.empty(), ());
  t = m_Types.back();
  m_Types.pop_back();
  return m_Types.empty();
}

bool FeatureParams::PopExactType(uint32_t t)
{
  m_Types.erase(remove(m_Types.begin(), m_Types.end(), t), m_Types.end());
  return m_Types.empty();
}

bool FeatureParams::IsTypeExist(uint32_t t) const
{
  return (find(m_Types.begin(), m_Types.end(), t) != m_Types.end());
}

bool FeatureParams::operator == (FeatureParams const & rhs) const
{
  return (FeatureParamsBase::operator ==(rhs) &&
          m_Types == rhs.m_Types &&
          GetGeomType() == rhs.GetGeomType());
}

bool FeatureParams::CheckValid() const
{
  CHECK(!m_Types.empty() && m_Types.size() <= max_types_count, ());
  CHECK_NOT_EQUAL(m_geomType, 0xFF, ());

  return FeatureParamsBase::CheckValid();
}

uint8_t FeatureParams::GetHeader() const
{
  uint8_t header = static_cast<uint8_t>(m_Types.size() - 1);

  if (!name.IsEmpty())
    header |= HEADER_HAS_NAME;

  if (layer != 0)
    header |= HEADER_HAS_LAYER;

  uint8_t const typeMask = GetTypeMask();
  header |= typeMask;

  // Geometry type for additional info is only one.
  switch (GetTypeMask())
  {
  case HEADER_GEOM_POINT: if (rank != 0) header |= HEADER_HAS_ADDINFO; break;
  case HEADER_GEOM_LINE: if (!ref.empty()) header |= HEADER_HAS_ADDINFO; break;
  case HEADER_GEOM_AREA:
  case HEADER_GEOM_POINT_EX:
    if (!house.IsEmpty()) header |= HEADER_HAS_ADDINFO; break;
  }

  return header;
}

uint32_t FeatureParams::GetIndexForType(uint32_t t)
{
  return classif().GetIndexForType(t);
}

uint32_t FeatureParams::GetTypeForIndex(uint32_t i)
{
  return classif().GetTypeForIndex(i);
}

string DebugPrint(FeatureParams const & p)
{
  Classificator const & c = classif();

  ostringstream out;

  out << "Types: ";
  for (size_t i = 0; i < p.m_Types.size(); ++i)
    out << c.GetReadableObjectName(p.m_Types[i]) << "; ";

  return (p.DebugString() + out.str());
}
