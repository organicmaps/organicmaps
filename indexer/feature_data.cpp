#include "indexer/feature_data.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <functional>
#include <vector>

using namespace feature;
using namespace std;
using namespace std::placeholders;

////////////////////////////////////////////////////////////////////////////////////
// TypesHolder implementation
////////////////////////////////////////////////////////////////////////////////////

namespace feature
{
string DebugPrint(TypesHolder const & holder)
{
  Classificator const & c = classif();
  string s;
  for (uint32_t const type : holder)
    s += c.GetReadableObjectName(type) + " ";
  if (!s.empty())
    s.pop_back();
  return s;
}

TypesHolder::TypesHolder(FeatureType & f) : m_size(0), m_geoType(f.GetFeatureType())
{
  f.ForEachType([this](uint32_t type)
  {
    Add(type);
  });
}

void TypesHolder::Remove(uint32_t type)
{
  UNUSED_VALUE(RemoveIf(base::EqualFunctor<uint32_t>(type)));
}

bool TypesHolder::Equals(TypesHolder const & other) const
{
  vector<uint32_t> my(this->begin(), this->end());
  vector<uint32_t> his(other.begin(), other.end());

  sort(::begin(my), ::end(my));
  sort(::begin(his), ::end(his));

  return my == his;
}
}  // namespace feature

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
      { "building:part" },
      { "hwtag" },
      { "psurface" },
      { "internet_access" },
      { "wheelchair" },
      { "sponsored" },
      { "entrance" },
      { "cuisine" },
    };

    AddTypes(arr1);
    m_count1 = m_types.size();

    // 2-arity
    char const * arr2[][2] = {
      { "amenity", "atm" },
      { "amenity", "bench" },
      { "amenity", "shelter" },
      { "amenity", "toilets" },
      { "building", "address" },
      { "building", "has_parts" },
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
}  // namespace

namespace feature
{
uint8_t CalculateHeader(size_t const typesCount, uint8_t const headerGeomType,
                        FeatureParamsBase const & params)
{
  ASSERT(typesCount != 0, ("Feature should have at least one type."));
  uint8_t header = static_cast<uint8_t>(typesCount - 1);

  if (!params.name.IsEmpty())
    header |= HEADER_HAS_NAME;

  if (params.layer != 0)
    header |= HEADER_HAS_LAYER;

  header |= headerGeomType;

  // Geometry type for additional info is only one.
  switch (headerGeomType)
  {
  case HEADER_GEOM_POINT:
    if (params.rank != 0)
      header |= HEADER_HAS_ADDINFO;
    break;
  case HEADER_GEOM_LINE:
    if (!params.ref.empty())
      header |= HEADER_HAS_ADDINFO;
    break;
  case HEADER_GEOM_AREA:
  case HEADER_GEOM_POINT_EX:
    if (!params.house.IsEmpty())
      header |= HEADER_HAS_ADDINFO;
    break;
  }

  return header;
}

void TypesHolder::SortBySpec()
{
  if (m_size < 2)
    return;

  // Put "very common" types to the end of possible PP-description types.
  static UselessTypesChecker checker;
  UNUSED_VALUE(base::RemoveIfKeepValid(begin(), end(), [](uint32_t t) { return checker(t); }));
}

vector<string> TypesHolder::ToObjectNames() const
{
  vector<string> result;
  for (auto const type : *this)
    result.push_back(classif().GetReadableObjectName(type));
  return result;
}
}  // namespace feature

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
  string const utf8name = DebugPrint(name);
  return ((!utf8name.empty() ? "Name:" + utf8name : "") +
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
// https://taginfo.openstreetmap.org/keys/addr%3Ahousename#values
bool IsDummyName(string const & s)
{
  return (s.empty() ||
          s == "Bloc" || s == "bloc" ||
          s == "жилой дом" ||
          s == "Edificio" || s == "edificio");
}

}

/////////////////////////////////////////////////////////////////////////////////////////
// FeatureParams implementation
/////////////////////////////////////////////////////////////////////////////////////////

void FeatureParams::ClearName()
{
  name.Clear();
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
  if (IsDummyName(s) || name.FindString(s) != StringUtf8Multilang::kUnsupportedLanguageCode)
    return false;

  // Most names are house numbers by statistics.
  if (house.IsEmpty() && AddHouseNumber(s))
    return true;

  // If we got a clear number, replace the house number with it.
  // Example: housename=16th Street, housenumber=34
  if (strings::is_number(s))
  {
    string housename(house.Get());
    if (AddHouseNumber(s))
    {
      // Duplicating code to avoid changing the method header.
      string dummy;
      if (!name.GetString(StringUtf8Multilang::kDefaultCode, dummy))
        name.AddString(StringUtf8Multilang::kDefaultCode, housename);
      return true;
    }
  }

  // Add as a default name if we don't have it yet.
  string dummy;
  if (!name.GetString(StringUtf8Multilang::kDefaultCode, dummy))
  {
    name.AddString(StringUtf8Multilang::kDefaultCode, s);
    return true;
  }

  return false;
}

bool FeatureParams::AddHouseNumber(string houseNumber)
{
  ASSERT(!houseNumber.empty(), ("This check should be done by the caller."));
  ASSERT_NOT_EQUAL(houseNumber.front(), ' ', ("Trim should be done by the caller."));

  // Negative house numbers are not supported.
  if (houseNumber.front() == '-' || houseNumber.find(u8"－") == 0)
    return false;

  // Replace full-width digits, mostly in Japan, by ascii-ones.
  strings::NormalizeDigits(houseNumber);

  // Remove leading zeroes from house numbers.
  // It's important for debug checks of serialized-deserialized feature.
  size_t i = 0;
  while (i + 1 < houseNumber.size() && houseNumber[i] == '0')
    ++i;
  houseNumber.erase(0, i);

  if (any_of(houseNumber.cbegin(), houseNumber.cend(), IsDigit))
  {
    house.Set(houseNumber);
    return true;
  }
  return false;
}

void FeatureParams::AddStreet(string s)
{
  // Replace \n with spaces because we write addresses to txt file.
  replace(s.begin(), s.end(), '\n', ' ');

  m_addrTags.Add(AddressData::STREET, s);
}

void FeatureParams::AddAddress(string const & s)
{
  size_t i = s.find_first_of("\t ");
  if (i != string::npos)
  {
    string const house = s.substr(0, i);
    if (feature::IsHouseNumber(house))
    {
      AddHouseNumber(house);
      i = s.find_first_not_of("\t ", i);
    }
    else
    {
      i = 0;
    }
  }
  else
  {
    i = 0;
  }

  AddStreet(s.substr(i));
}

void FeatureParams::AddPlace(string const & s)
{
  m_addrTags.Add(AddressData::PLACE, s);
}

void FeatureParams::AddPostcode(string const & s)
{
  m_addrTags.Add(AddressData::POSTCODE, s);
}

bool FeatureParams::FormatFullAddress(m2::PointD const & pt, string & res) const
{
  string const street = GetStreet();
  if (!street.empty() && !house.IsEmpty())
  {
    res = street + "|" + house.Get() + "|"
        + strings::to_string_dac(MercatorBounds::YToLat(pt.y), 8) + "|"
        + strings::to_string_dac(MercatorBounds::XToLon(pt.x), 8) + '\n';
    return true;
  }

  return false;
}

string FeatureParams::GetStreet() const
{
  return m_addrTags.Get(AddressData::STREET);
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

  for (size_t i = 0; i < m_types.size(); ++i)
  {
    uint32_t t = m_types[i];
    ftype::TruncValue(t, 2);
    if (t == src)
    {
      m_types[i] = dest;
      break;
    }
  }
}

void FeatureParams::AddTypes(FeatureParams const & rhs, uint32_t skipType2)
{
  if (skipType2 == 0)
  {
    m_types.insert(m_types.end(), rhs.m_types.begin(), rhs.m_types.end());
  }
  else
  {
    for (size_t i = 0; i < rhs.m_types.size(); ++i)
    {
      uint32_t t = rhs.m_types[i];
      ftype::TruncValue(t, 2);
      if (t != skipType2)
        m_types.push_back(rhs.m_types[i]);
    }
  }
}

bool FeatureParams::FinishAddingTypes()
{
  static uint32_t const boundary = classif().GetTypeByPath({ "boundary", "administrative" });

  vector<uint32_t> newTypes;
  newTypes.reserve(m_types.size());

  for (size_t i = 0; i < m_types.size(); ++i)
  {
    uint32_t candidate = m_types[i];

    // Assume that classificator types are equal if they are equal for 2-arity dimension
    // (e.g. "place-city-capital" is equal to "place-city" and we leave the longest one "place-city-capital").
    // The only exception is "boundary-administrative" type.

    uint32_t type = m_types[i];
    ftype::TruncValue(type, 2);
    if (type != boundary)
    {
      // Find all equal types (2-arity).
      auto j = base::RemoveIfKeepValid(m_types.begin() + i + 1, m_types.end(), [type] (uint32_t t)
      {
        ftype::TruncValue(t, 2);
        return (type == t);
      });

      // Choose the best type from equals by arity level.
      for (auto k = j; k != m_types.end(); ++k)
        if (ftype::GetLevel(*k) > ftype::GetLevel(candidate))
          candidate = *k;

      // Delete equal types.
      m_types.erase(j, m_types.end());
    }

    newTypes.push_back(candidate);
  }

  // Remove duplicated types.
  sort(newTypes.begin(), newTypes.end());
  newTypes.erase(unique(newTypes.begin(), newTypes.end()), newTypes.end());

  m_types.swap(newTypes);

  if (m_types.size() > kMaxTypesCount)
    m_types.resize(kMaxTypesCount);

  // Patch fix that removes house number from localities.
  if (!house.IsEmpty() && ftypes::IsLocalityChecker::Instance()(m_types))
  {
    LOG(LINFO, ("Locality with house number", *this));
    house.Clear();
  }

  return !m_types.empty();
}

void FeatureParams::SetType(uint32_t t)
{
  m_types.clear();
  m_types.push_back(t);
}

bool FeatureParams::PopAnyType(uint32_t & t)
{
  CHECK(!m_types.empty(), ());
  t = m_types.back();
  m_types.pop_back();
  return m_types.empty();
}

bool FeatureParams::PopExactType(uint32_t t)
{
  m_types.erase(remove(m_types.begin(), m_types.end(), t), m_types.end());
  return m_types.empty();
}

bool FeatureParams::IsTypeExist(uint32_t t) const
{
  return (find(m_types.begin(), m_types.end(), t) != m_types.end());
}

uint32_t FeatureParams::FindType(uint32_t comp, uint8_t level) const
{
  for (uint32_t const type : m_types)
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
  CHECK(!m_types.empty() && m_types.size() <= kMaxTypesCount, ());
  CHECK_NOT_EQUAL(m_geomType, 0xFF, ());

  return FeatureParamsBase::CheckValid();
}

uint8_t FeatureParams::GetHeader() const
{
  return CalculateHeader(m_types.size(), GetTypeMask(), *this);
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

  string res = "Types: ";
  for (size_t i = 0; i < p.m_types.size(); ++i)
    res = res + c.GetReadableObjectName(p.m_types[i]) + "; ";

  return (res + p.DebugString());
}
