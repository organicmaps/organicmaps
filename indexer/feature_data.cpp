#include "indexer/feature_data.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "base/stl_add.hpp"

#include "std/bind.hpp"


using namespace feature;

////////////////////////////////////////////////////////////////////////////////////
// TypesHolder implementation
////////////////////////////////////////////////////////////////////////////////////

TypesHolder::TypesHolder(FeatureBase const & f)
: m_size(0), m_geoType(f.GetFeatureType())
{
  f.ForEachType([this](uint32_t type)
  {
    this->operator()(type);
  });
}

string TypesHolder::DebugPrint() const
{
  Classificator const & c = classif();

  string s;
  for (uint32_t t : *this)
    s += c.GetFullObjectName(t) + "  ";
  return s;
}

void TypesHolder::Remove(uint32_t t)
{
  (void) RemoveIf(EqualFunctor<uint32_t>(t));
}

namespace
{

class UselessTypesChecker
{
  vector<uint32_t> m_types;
  size_t m_count1;

  template <size_t N, size_t M>
  void AddTypes(char const * (&arr)[N][M])
  {
    Classificator const & c = classif();

    for (size_t i = 0; i < N; ++i)
      m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + M)));
  }

public:
  UselessTypesChecker()
  {
    // Fill types that will be taken into account last,
    // when we have many types for POI.

    // 1-arity
    char const * arr1[][1] = {
      { "building" },
      { "hwtag" },
      { "internet_access" },
    };

    AddTypes(arr1);
    m_count1 = m_types.size();

    // 2-arity
    char const * arr2[][2] = {
      { "amenity", "atm" }
    };

    AddTypes(arr2);
  }

  bool operator() (uint32_t t) const
  {
    auto const end1 = m_types.begin() + m_count1;

    // check 2-arity types
    ftype::TruncValue(t, 2);
    if (find(end1, m_types.end(), t) != m_types.end())
      return true;

    // check 1-arity types
    ftype::TruncValue(t, 1);
    if (find(m_types.begin(), end1, t) != end1)
      return true;

    return false;
  }
};

}

void TypesHolder::SortBySpec()
{
  if (m_size < 2)
    return;

  // Put "very common" types to the end of possible PP-description types.
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
  name.GetString(StringUtf8Multilang::DEFAULT_CODE, utf8name);

  return ((!utf8name.empty() ? "Name:" + utf8name : "") +
          (" Layer:" + DebugPrint(layer)) +
          (rank != 0 ? " Rank:" + DebugPrint(rank) : "") +
          (!house.IsEmpty() ? " House:" + house.Get() : "") +
          (!ref.empty() ? " Ref:" + ref : ""));
}

bool FeatureParamsBase::IsEmptyNames() const
{
  return name.IsEmpty() && house.IsEmpty() && ref.empty();
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

  // The "default" new name will replace the old one if any (e.g. from AddHouseName call).
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
        + strings::to_string_dac(MercatorBounds::YToLat(pt.y), 8) + "|"
        + strings::to_string_dac(MercatorBounds::XToLon(pt.x), 8) + '\n';
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

void FeatureParams::SetRwSubwayType(char const * cityName)
{
  Classificator const & c = classif();

  static uint32_t const src = c.GetTypeByPath({"railway", "station"});
  uint32_t const dest = c.GetTypeByPath({"railway", "station", "subway", cityName});

  for (size_t i = 0; i < m_Types.size(); ++i)
  {
    uint32_t t = m_Types[i];
    ftype::TruncValue(t, 2);
    if (t == src)
    {
      m_Types[i] = dest;
      break;
    }
  }
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
  static uint32_t const boundary = classif().GetTypeByPath({ "boundary", "administrative" });

  vector<uint32_t> newTypes;
  newTypes.reserve(m_Types.size());

  for (size_t i = 0; i < m_Types.size(); ++i)
  {
    uint32_t candidate = m_Types[i];

    // Assume that classificator types are equal if they are equal for 2-arity dimension
    // (e.g. "place-city-capital" is equal to "place-city" and we leave the longest one "place-city-capital").
    // The only exception is "boundary-administrative" type.

    uint32_t type = m_Types[i];
    ftype::TruncValue(type, 2);
    if (type != boundary)
    {
      // Find all equal types (2-arity).
      auto j = RemoveIfKeepValid(m_Types.begin() + i + 1, m_Types.end(), [type] (uint32_t t)
      {
        ftype::TruncValue(t, 2);
        return (type == t);
      });

      // Choose the best type from equals by arity level.
      for (auto k = j; k != m_Types.end(); ++k)
        if (ftype::GetLevel(*k) > ftype::GetLevel(candidate))
          candidate = *k;

      // Delete equal types.
      m_Types.erase(j, m_Types.end());
    }

    newTypes.push_back(candidate);
  }

  // Remove duplicated types.
  sort(newTypes.begin(), newTypes.end());
  newTypes.erase(unique(newTypes.begin(), newTypes.end()), newTypes.end());

  m_Types.swap(newTypes);

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

uint32_t FeatureParams::FindType(uint32_t comp, uint8_t level) const
{
  for (uint32_t const type : m_Types)
  {
    uint32_t t = type;
    ftype::TruncValue(t, level);
    if (t == comp)
      return type;
  }
  return ftype::GetEmptyValue();
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

  string res = "Types";
  for (size_t i = 0; i < p.m_Types.size(); ++i)
    res += (" : " + c.GetReadableObjectName(p.m_Types[i]));

  return (res + p.DebugString());
}
