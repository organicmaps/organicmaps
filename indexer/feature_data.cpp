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
#include <sstream>
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

TypesHolder::TypesHolder(FeatureType & f) : m_size(0), m_geomType(f.GetGeomType())
{
  f.ForEachType([this](uint32_t type)
  {
    Add(type);
  });
}

// static
TypesHolder TypesHolder::FromTypesIndexes(std::vector<uint32_t> const & indexes)
{
  TypesHolder result;
  for (auto index : indexes)
  {
    result.Add(classif().GetTypeForIndex(index));
  }

  return result;
}

void TypesHolder::Remove(uint32_t type)
{
  UNUSED_VALUE(RemoveIf(base::EqualFunctor<uint32_t>(type)));
}

bool TypesHolder::Equals(TypesHolder const & other) const
{
  if (m_size != other.m_size)
    return false;

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
public:
  static UselessTypesChecker const & Instance()
  {
    static UselessTypesChecker const inst;
    return inst;
  }

  bool operator()(uint32_t t) const
  {
    ftype::TruncValue(t, 2);
    if (find(m_types2.begin(), m_types2.end(), t) != m_types2.end())
      return true;

    ftype::TruncValue(t, 1);
    if (find(m_types1.begin(), m_types1.end(), t) != m_types1.end())
      return true;

    return false;
  }

private:
  UselessTypesChecker()
  {
    // Fill types that will be taken into account last,
    // when we have many types for POI.
    vector<vector<string>> const types = {
        // 1-arity
        {"building"},
        {"building:part"},
        {"hwtag"},
        {"psurface"},
        {"internet_access"},
        {"wheelchair"},
        {"sponsored"},
        {"entrance"},
        {"cuisine"},
        {"recycling"},
        {"area:highway"},
        {"earthquake:damage"},
        // 2-arity
        {"amenity", "atm"},
        {"amenity", "bench"},
        {"amenity", "shelter"},
        {"amenity", "toilets"},
        {"amenity", "drinking_water"},
        {"building", "address"},
        {"building", "has_parts"},
    };

    Classificator const & c = classif();
    for (auto const & type : types)
    {
      if (type.size() == 1)
        m_types1.push_back(c.GetTypeByPath(type));
      else if (type.size() == 2)
        m_types2.push_back(c.GetTypeByPath(type));
      else
        CHECK(false, ());
    }
  }

  vector<uint32_t> m_types1;
  vector<uint32_t> m_types2;
};
}  // namespace

namespace feature
{
uint8_t CalculateHeader(size_t const typesCount, HeaderGeomType const headerGeomType,
                        FeatureParamsBase const & params)
{
  ASSERT(typesCount != 0, ("Feature should have at least one type."));
  uint8_t header = static_cast<uint8_t>(typesCount - 1);

  if (!params.name.IsEmpty())
    header |= HEADER_MASK_HAS_NAME;

  if (params.layer != 0)
    header |= HEADER_MASK_HAS_LAYER;

  header |= static_cast<uint8_t>(headerGeomType);

  // Geometry type for additional info is only one.
  switch (headerGeomType)
  {
  case HeaderGeomType::Point:
    if (params.rank != 0)
      header |= HEADER_MASK_HAS_ADDINFO;
    break;
  case HeaderGeomType::Line:
    if (!params.ref.empty())
      header |= HEADER_MASK_HAS_ADDINFO;
    break;
  case HeaderGeomType::Area:
  case HeaderGeomType::PointEx:
    if (!params.house.IsEmpty())
      header |= HEADER_MASK_HAS_ADDINFO;
    break;
  }

  return header;
}

void TypesHolder::SortBySpec()
{
  if (m_size < 2)
    return;

  // Put "very common" types to the end of possible PP-description types.
  auto const & checker = UselessTypesChecker::Instance();
  UNUSED_VALUE(base::RemoveIfKeepValid(begin(), end(), checker));
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

bool FeatureParamsBase::IsValid() const
{
  return layer > LAYER_LOW && layer < LAYER_HIGH;
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

void FeatureParams::SetGeomType(feature::GeomType t)
{
  switch (t)
  {
  case GeomType::Point: m_geomType = HeaderGeomType::Point; break;
  case GeomType::Line: m_geomType = HeaderGeomType::Line; break;
  case GeomType::Area: m_geomType = HeaderGeomType::Area; break;
  default: ASSERT(false, ());
  }
}

void FeatureParams::SetGeomTypePointEx()
{
  ASSERT(m_geomType == HeaderGeomType::Point ||
         m_geomType == HeaderGeomType::PointEx, ());
  ASSERT(!house.IsEmpty(), ());

  m_geomType = HeaderGeomType::PointEx;
}

feature::GeomType FeatureParams::GetGeomType() const
{
  CHECK(IsValid(), ());
  switch (*m_geomType)
  {
  case HeaderGeomType::Line: return GeomType::Line;
  case HeaderGeomType::Area: return GeomType::Area;
  default: return GeomType::Point;
  }
}

HeaderGeomType FeatureParams::GetHeaderGeomType() const
{
  CHECK(IsValid(), ());
  return *m_geomType;
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
  base::SortUnique(m_types);

  if (m_types.size() > kMaxTypesCount)
  {
    // Put common types to the end to leave the most important types.
    auto const & checker = UselessTypesChecker::Instance();
    UNUSED_VALUE(base::RemoveIfKeepValid(m_types.begin(), m_types.end(), checker));
    m_types.resize(kMaxTypesCount);
    sort(m_types.begin(), m_types.end());
  }

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

bool FeatureParams::IsTypeExist(uint32_t comp, uint8_t level) const
{
  return FindType(comp, level) != ftype::GetEmptyValue();
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

bool FeatureParams::IsValid() const
{
  if (m_types.empty() || m_types.size() > kMaxTypesCount || !m_geomType)
    return false;

  return FeatureParamsBase::IsValid();
}

uint8_t FeatureParams::GetHeader() const
{
  return CalculateHeader(m_types.size(), GetHeaderGeomType(), *this);
}

uint32_t FeatureParams::GetIndexForType(uint32_t t)
{
  return classif().GetIndexForType(t);
}

uint32_t FeatureParams::GetTypeForIndex(uint32_t i)
{
  return classif().GetTypeForIndex(i);
}

void FeatureBuilderParams::AddStreet(string s)
{
  // Replace \n with spaces because we write addresses to txt file.
  replace(s.begin(), s.end(), '\n', ' ');

  m_addrTags.Add(AddressData::Type::Street, s);
}

void FeatureBuilderParams::AddPostcode(string const & s)
{
  m_addrTags.Add(AddressData::Type::Postcode, s);
}

string DebugPrint(FeatureParams const & p)
{
  Classificator const & c = classif();

  string res = "Types: ";
  for (size_t i = 0; i < p.m_types.size(); ++i)
    res = res + c.GetReadableObjectName(p.m_types[i]) + "; ";

  return (res + p.DebugString());
}

string DebugPrint(FeatureBuilderParams const & p)
{
  ostringstream oss;
  oss << "ReversedGeometry: " << (p.GetReversedGeometry() ? "true" : "false") << "; ";
  oss << DebugPrint(p.GetMetadata()) << "; ";
  oss << DebugPrint(p.GetAddressData()) << "; ";
  oss << DebugPrint(FeatureParams(p));
  return oss.str();
}
