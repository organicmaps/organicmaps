#include "indexer/feature_data.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

using namespace feature;

////////////////////////////////////////////////////////////////////////////////////
// TypesHolder implementation
////////////////////////////////////////////////////////////////////////////////////

namespace feature
{
using namespace std;

template <class ContT> string TypesToString(ContT const & holder)
{
  Classificator const & c = classif();
  string s;
  for (uint32_t const type : holder)
    s += c.GetReadableObjectName(type) + " ";
  if (!s.empty())
    s.pop_back();
  return s;
}

std::string DebugPrint(TypesHolder const & holder)
{
  return TypesToString(holder);
}

TypesHolder::TypesHolder(FeatureType & f) : m_size(0), m_geomType(f.GetGeomType())
{
  f.ForEachType([this](uint32_t type)
  {
    Add(type);
  });
}

bool TypesHolder::HasWithSubclass(uint32_t type) const
{
  uint8_t const level = ftype::GetLevel(type);
  for (uint32_t t : *this)
  {
    ftype::TruncValue(t, level);
    if (t == type)
      return true;
  }
  return false;
}

void TypesHolder::Remove(uint32_t type)
{
  UNUSED_VALUE(RemoveIf(base::EqualFunctor<uint32_t>(type)));
}

bool TypesHolder::Equals(TypesHolder const & other) const
{
  if (m_size != other.m_size)
    return false;

  // Dynamic vector + sort for kMaxTypesCount array is a huge overhead.

  auto const b = begin();
  auto const e = end();
  for (auto t : other)
  {
    if (std::find(b, e, t) == e)
      return false;
  }
  return true;
}

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

  /// @return Type score, less is better.
  uint8_t Score(uint32_t t) const
  {
    ftype::TruncValue(t, 2);
    if (IsIn(2, t))
      return 3;

    ftype::TruncValue(t, 1);
    if (IsIn(1, t))
      return 2;

    if (IsIn(0, t))
      return 1;

    return 0;
  }

  template <class ContT> void SortUselessToEnd(ContT & cont) const
  {
    // Put "very common" types to the end of possible PP-description types.
    std::stable_sort(cont.begin(), cont.end(), [this](uint32_t t1, uint32_t t2)
    {
      return Score(t1) < Score(t2);
    });
  }

private:
  UselessTypesChecker()
  {
    // Fill types that will be taken into account last,
    // when we have many types for POI.
    base::StringIL const types1[] = {
        // 1-arity
        {"building:part"},
        {"hwtag"},
        {"psurface"},
        {"internet_access"},
        {"organic"},
        {"wheelchair"},
        {"cuisine"},
        {"recycling"},
        {"area:highway"},
        {"fee"},
    };

    Classificator const & c = classif();

    m_types[0].push_back(c.GetTypeByPath({"building"}));

    m_types[1].reserve(std::size(types1));
    for (auto const & type : types1)
      m_types[1].push_back(c.GetTypeByPath(type));

    // Put _most_ useless types here, that are not fit in the arity logic above.
    // This change is for generator, to eliminate "lit" type first when max types count exceeded.
    m_types[2].push_back(c.GetTypeByPath({"hwtag", "lit"}));

    for (auto & v : m_types)
      std::sort(v.begin(), v.end());
  }

  bool IsIn(uint8_t idx, uint32_t t) const
  {
    return std::binary_search(m_types[idx].begin(), m_types[idx].end(), t);
  }

  vector<uint32_t> m_types[3];
};
} // namespace

uint8_t CalculateHeader(size_t const typesCount, HeaderGeomType const headerGeomType,
                        FeatureParamsBase const & params)
{
  ASSERT(typesCount != 0, ("Feature should have at least one type."));
  ASSERT_LESS_OR_EQUAL(typesCount, kMaxTypesCount, ());
  uint8_t header = static_cast<uint8_t>(typesCount - 1);

  if (!params.name.IsEmpty())
    header |= HEADER_MASK_HAS_NAME;

  if (params.layer != LAYER_EMPTY)
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

void TypesHolder::SortByUseless()
{
  UselessTypesChecker::Instance().SortUselessToEnd(*this);
}

void TypesHolder::SortBySpec()
{
  auto const & cl = classif();
  auto const getPriority = [&cl](uint32_t type)
  {
    return cl.GetObject(type)->GetMaxOverlaysPriority();
  };

  auto const & checker = UselessTypesChecker::Instance();

  std::stable_sort(begin(), end(), [&checker, &getPriority](uint32_t t1, uint32_t t2)
  {
    int const p1 = getPriority(t1);
    int const p2 = getPriority(t2);
    if (p1 != p2)
      return p1 > p2;

    // Score - less is better.
    return checker.Score(t1) < checker.Score(t2);
  });
}

vector<string> TypesHolder::ToObjectNames() const
{
  Classificator const & c = classif();
  vector<string> result;
  for (uint32_t const type : *this)
    result.push_back(c.GetReadableObjectName(type));
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

bool FeatureParamsBase::SetDefaultNameIfEmpty(std::string const & s)
{
  std::string_view existing;
  if (name.GetString(StringUtf8Multilang::kDefaultCode, existing))
    return existing == s;

  name.AddString(StringUtf8Multilang::kDefaultCode, s);
  return true;
}

bool FeatureParamsBase::operator == (FeatureParamsBase const & rhs) const
{
  return (name == rhs.name && house == rhs.house && ref == rhs.ref &&
          layer == rhs.layer && rank == rhs.rank);
}

bool FeatureParamsBase::IsValid() const
{
  return layer >= LAYER_LOW && layer <= LAYER_HIGH;
}

string FeatureParamsBase::DebugString() const
{
  string const utf8name = DebugPrint(name);
  return ((!utf8name.empty() ? "Name:" + utf8name : "") +
          (layer != LAYER_EMPTY ? " Layer:" + DebugPrint((int)layer) : "") +
          (rank != 0 ? " Rank:" + DebugPrint((int)rank) : "") +
          (!house.IsEmpty() ? " House:" + house.Get() : "") +
          (!ref.empty() ? " Ref:" + ref : ""));
}

bool FeatureParamsBase::IsEmptyNames() const
{
  return name.IsEmpty() && house.IsEmpty() && ref.empty();
}

namespace
{

bool IsDummyName(string_view s)
{
  return s.empty();
}

} // namespace

/////////////////////////////////////////////////////////////////////////////////////////
// FeatureParams implementation
/////////////////////////////////////////////////////////////////////////////////////////

void FeatureParams::ClearName()
{
  name.Clear();
}

bool FeatureParams::AddName(string_view lang, string_view s)
{
  if (IsDummyName(s))
    return false;

  // The "default" new name will replace the old one if any (e.g. from SetHouseNumberAndHouseName call).
  name.AddString(lang, s);
  return true;
}

bool FeatureParams::LooksLikeHouseNumber(std::string const & hn)
{
  // Very naive implementation to _lightly_ promote hn -> name (for search index) if suitable.
  /// @todo Conform with search::LooksLikeHouseNumber.

  ASSERT(!hn.empty(), ());
  size_t const sz = hn.size();
  return strings::IsASCIIDigit(hn[0]) ||
         (sz == 1 && strings::IsASCIILatin(hn[0])) ||
         std::count_if(hn.begin(), hn.end(), &strings::IsASCIIDigit) > 0.2 * sz;
}

char const * FeatureParams::kHNLogTag = "HNLog";

void FeatureParams::SetHouseNumberAndHouseName(std::string houseNumber, std::string houseName)
{
  if (IsDummyName(houseName) || name.FindString(houseName) != StringUtf8Multilang::kUnsupportedLanguageCode)
    houseName.clear();

  if (houseName.empty() && houseNumber.empty())
    return;

  if (houseName.empty())
    AddHouseNumber(houseNumber);
  else if (houseNumber.empty())
    AddHouseNumber(houseName);
  else
  {
    if (!LooksLikeHouseNumber(houseNumber) && LooksLikeHouseNumber(houseName))
    {
      LOG(LWARNING, (kHNLogTag, "Fancy house name:", houseName, "and number:", houseNumber));
      houseNumber.swap(houseName);
    }

    AddHouseNumber(std::move(houseNumber));
    SetDefaultNameIfEmpty(houseName);
  }
}

bool FeatureParams::AddHouseNumber(string houseNumber)
{
  ASSERT(!houseNumber.empty(), ());

  // Negative house numbers are not supported.
  if (houseNumber.front() == '-' || houseNumber.find("－") == 0)
  {
    LOG(LWARNING, (kHNLogTag, "Negative house number:", houseNumber));
    return false;
  }

  // Replace full-width digits, mostly in Japan, by ascii-ones.
  strings::NormalizeDigits(houseNumber);

  // Assign house number as-is to create building-address if needed.
  // Final checks are made in FeatureBuilder::PreSerialize().
  house.Set(houseNumber);
  return true;
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
  ASSERT(m_geomType == HeaderGeomType::Point || m_geomType == HeaderGeomType::PointEx, ());
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
    if (ftype::Trunc(m_types[i], 2) == src)
    {
      m_types[i] = dest;
      break;
    }
  }
}

FeatureParams::TypesResult FeatureParams::FinishAddingTypesEx()
{
  base::SortUnique(m_types);

  TypesResult res = TYPES_GOOD;

  if (m_types.size() > kMaxTypesCount)
  {
    UselessTypesChecker::Instance().SortUselessToEnd(m_types);

    m_types.resize(kMaxTypesCount);
    sort(m_types.begin(), m_types.end());

    res = TYPES_EXCEED_MAX;
  }

  // Patch fix that removes house number from localities.
  /// @todo move this fix elsewhere (osm2type.cpp?)
  if (!house.IsEmpty() && ftypes::IsLocalityChecker::Instance()(m_types))
    house.Clear();

  return (m_types.empty() ? TYPES_EMPTY : res);
}

std::string FeatureParams::PrintTypes()
{
  base::SortUnique(m_types);
  UselessTypesChecker::Instance().SortUselessToEnd(m_types);
  return TypesToString(m_types);
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
  return base::IsExist(m_types, t);
}

bool FeatureParams::IsTypeExist(uint32_t comp, uint8_t level) const
{
  return FindType(comp, level) != ftype::GetEmptyValue();
}

uint32_t FeatureParams::FindType(uint32_t comp, uint8_t level) const
{
  for (uint32_t const type : m_types)
  {
    if (ftype::Trunc(type, level) == comp)
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

void FeatureBuilderParams::SetStreet(string s)
{
  m_addrTags.Set(AddressData::Type::Street, std::move(s));
}

std::string_view FeatureBuilderParams::GetStreet() const
{
  return m_addrTags.Get(AddressData::Type::Street);
}

void FeatureBuilderParams::SetPostcode(string s)
{
  if (!s.empty())
    m_metadata.Set(Metadata::FMD_POSTCODE, std::move(s));
}

std::string_view FeatureBuilderParams::GetPostcode() const
{
  return m_metadata.Get(Metadata::FMD_POSTCODE);
}

namespace
{

// Define types that can't live together in a feature.
class YesNoTypes
{
  std::vector<std::pair<uint32_t, uint32_t>> m_types;

public:
  YesNoTypes()
  {
    // Remain first type and erase second in case of conflict.
    base::StringIL arr[][2] = {
      {{"hwtag", "yescar"}, {"hwtag", "nocar"}},
      {{"hwtag", "yesfoot"}, {"hwtag", "nofoot"}},
      {{"hwtag", "yesbicycle"}, {"hwtag", "nobicycle"}},
      {{"hwtag", "nobicycle"}, {"hwtag", "bidir_bicycle"}},
      {{"hwtag", "nobicycle"}, {"hwtag", "onedir_bicycle"}},
      {{"hwtag", "bidir_bicycle"}, {"hwtag", "onedir_bicycle"}},
      {{"wheelchair", "yes"}, {"wheelchair", "no"}},
    };

    auto const & cl = classif();
    for (auto const & p : arr)
      m_types.emplace_back(cl.GetTypeByPath(p[0]), cl.GetTypeByPath(p[1]));
  }

  bool RemoveInconsistent(std::vector<uint32_t> & types) const
  {
    size_t const szBefore = types.size();
    for (auto const & p : m_types)
    {
      uint32_t skip;
      bool found1 = false, found2 = false;
      for (uint32_t t : types)
      {
        if (t == p.first)
          found1 = true;
        if (t == p.second)
        {
          found2 = true;
          skip = t;
        }
      }

      if (found1 && found2)
        base::EraseIf(types, [skip](uint32_t t) { return skip == t; });
    }

    return szBefore != types.size();
  }
};

} // namespace

bool FeatureBuilderParams::RemoveInconsistentTypes()
{
  static YesNoTypes ynTypes;
  return ynTypes.RemoveInconsistent(m_types);
}

void FeatureBuilderParams::ClearPOIAttribs()
{
  ClearName();
  m_metadata.ClearPOIAttribs();
}

string DebugPrint(FeatureParams const & p)
{
  string res = "Types: " + TypesToString(p.m_types) + "; ";
  return (res + p.DebugString());
}

string DebugPrint(FeatureBuilderParams const & p)
{
  ostringstream oss;
  oss << "ReversedGeometry: " << (p.GetReversedGeometry() ? "true" : "false") << "; ";
  oss << DebugPrint(p.m_metadata) << "; ";
  oss << DebugPrint(p.m_addrTags) << "; ";
  oss << DebugPrint(static_cast<FeatureParams const &>(p));
  return oss.str();
}
