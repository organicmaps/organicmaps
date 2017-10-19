#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_mapping.hpp"

#include "coding/csv_reader.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>
#include <utility>

namespace ftraits
{
template <typename Base, typename Value>
class TraitsBase
{
public:
  static Value GetValue(feature::TypesHolder const & types)
  {
    static Base instance;

    auto const excluded = instance.m_excluded.Find(types);
    if (instance.m_excluded.IsValid(excluded))
      return Base::GetEmptyValue();

    auto const it = instance.m_matcher.Find(types);
    if (!instance.m_matcher.IsValid(it))
      return Base::GetEmptyValue();

    return it->second;
  }

protected:
  ftypes::HashMapMatcher<uint32_t, Value> m_matcher;
  ftypes::HashSetMatcher<uint32_t> m_excluded;
};

enum UGCType
{
  UGCTYPE_NONE = 0u,
  UGCTYPE_RATING = 1u << 0,   // 1
  UGCTYPE_REVIEWS = 1u << 1,  // 2
  UGCTYPE_DETAILS = 1u << 2   // 4
};

using UGCTypeMask = unsigned;
using UGCRatingCategories = std::vector<std::string>;

struct UGCItem
{
  UGCItem() {}
  UGCItem(UGCTypeMask m, UGCRatingCategories && c)
    : m_mask(m), m_categories(std::move(c))
  {
  }

  UGCTypeMask m_mask = UGCTYPE_NONE;
  UGCRatingCategories m_categories;
};

class UGC : public TraitsBase<UGC, UGCItem>
{
  friend class TraitsBase;

  std::array<UGCType, 3> const m_masks = {{UGCTYPE_RATING, UGCTYPE_REVIEWS, UGCTYPE_DETAILS}};

  UGC()
  {
    coding::CSVReader reader;
    auto const fileReader = GetPlatform().GetReader("ugc_types.csv");
    reader.Read(*fileReader, [this](coding::CSVReader::Row const & row) {
      size_t constexpr kTypePos = 0;
      size_t constexpr kCategoriesPos = 4;

      ASSERT_EQUAL(row.size(), 5, ());

      UGCItem item(ReadMasks(row), ParseByWhitespaces(row[kCategoriesPos]));
      auto typePath = ParseByWhitespaces(row[kTypePos]);

      if (IsUGCAvailable(item.m_mask))
        m_matcher.AppendType(std::move(typePath), std::move(item));
      else
        m_excluded.AppendType(std::move(typePath));
    });
  }

  UGCTypeMask ReadMasks(coding::CSVReader::Row const & row)
  {
    size_t constexpr kMasksBegin = 1;
    size_t constexpr kMasksEnd = 4;

    UGCTypeMask maskType = UGCTYPE_NONE;
    for (size_t i = kMasksBegin; i < kMasksEnd; i++)
    {
      int flag;
      if (!strings::to_int(row[i], flag))
      {
        LOG(LERROR, ("File ugc_types.csv must contain a bit mask of supported ugc traits!"));
        return UGCTYPE_NONE;
      }

      if (flag)
        maskType |= m_masks[i - 1];
    }

    return maskType;
  }

  std::vector<std::string> ParseByWhitespaces(std::string const & str)
  {
    std::istringstream iss(str);
    return {std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>()};
  }

public:
  static UGCItem const & GetEmptyValue()
  {
    static const UGCItem item;
    return item;
  }

  static bool IsUGCAvailable(UGCTypeMask mask) { return mask != UGCTYPE_NONE; }
  static bool IsRatingAvailable(UGCTypeMask mask) { return mask & UGCTYPE_RATING; }
  static bool IsReviewsAvailable(UGCTypeMask mask) { return mask & UGCTYPE_REVIEWS; }
  static bool IsDetailsAvailable(UGCTypeMask mask) { return mask & UGCTYPE_DETAILS; }

  static bool IsUGCAvailable(feature::TypesHolder const & types)
  {
    return IsUGCAvailable(GetValue(types).m_mask);
  }
  static bool IsRatingAvailable(feature::TypesHolder const & types)
  {
    return IsRatingAvailable(GetValue(types).m_mask);
  }
  static bool IsReviewsAvailable(feature::TypesHolder const & types)
  {
    return IsReviewsAvailable(GetValue(types).m_mask);
  }
  static bool IsDetailsAvailable(feature::TypesHolder const & types)
  {
    return IsDetailsAvailable(GetValue(types).m_mask);
  }
  static UGCRatingCategories GetCategories(feature::TypesHolder const & types)
  {
    return GetValue(types).m_categories;
  }
};

enum class WheelchairAvailability
{
  No,
  Yes,
  Limited
};

inline std::string DebugPrint(WheelchairAvailability wheelchair)
{
  switch (wheelchair)
  {
  case WheelchairAvailability::No: return "No";
  case WheelchairAvailability::Yes: return "Yes";
  case WheelchairAvailability::Limited: return "Limited";
  }
}

class Wheelchair : public TraitsBase<Wheelchair, WheelchairAvailability>
{
  friend class TraitsBase;

  using TypesInitializer = std::initializer_list<std::initializer_list<char const *>>;

  Wheelchair()
  {
    m_matcher.Append<TypesInitializer>({{"wheelchair", "no"}}, WheelchairAvailability::No);
    m_matcher.Append<TypesInitializer>({{"wheelchair", "yes"}}, WheelchairAvailability::Yes);
    m_matcher.Append<TypesInitializer>({{"wheelchair", "limited"}},
                                       WheelchairAvailability::Limited);
  }

public:
  static WheelchairAvailability GetEmptyValue() { return WheelchairAvailability::No; }
};

}  // namespace ftraits
