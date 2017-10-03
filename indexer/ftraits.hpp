#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_mapping.hpp"

#include "coding/csv_file_reader.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>

namespace ftraits
{
template <typename Base, typename Value, Value notFound>
class TraitsBase
{
public:
  static Value GetValue(feature::TypesHolder const & types)
  {
    static Base instance;
    auto const it = instance.m_matcher.Find(types);
    if (!instance.m_matcher.IsValid(it))
      return notFound;

    return it->second;
  }

protected:
  ftypes::HashMapMatcher<uint32_t, Value> m_matcher;
};

enum UGCType
{
  UGCTYPE_NONE = 0u,
  UGCTYPE_RATING = 1u << 0,   // 1
  UGCTYPE_REVIEWS = 1u << 1,  // 2
  UGCTYPE_DETAILS = 1u << 2   // 4
};

using UGCTypeMask = unsigned;

class UGC : public TraitsBase<UGC, UGCTypeMask, UGCTYPE_NONE>
{
  friend class TraitsBase;

  std::array<UGCType, 3> const m_masks = {{UGCTYPE_RATING, UGCTYPE_REVIEWS, UGCTYPE_DETAILS}};

  UGC()
  {
    coding::CSVReader reader;
    auto const filePath = GetPlatform().ReadPathForFile("ugc_types.csv", "wr");
    reader.ReadLineByLine(filePath, [this](std::vector<std::string> const & line) {
      auto const lineSize = line.size();
      ASSERT_EQUAL(lineSize, 4, ());
      ASSERT_EQUAL(lineSize - 1, m_masks.size(), ());

      UGCTypeMask maskType = UGCTYPE_NONE;
      for (size_t i = 1; i < lineSize; i++)
      {
        int flag;
        if (!strings::to_int(line[i], flag))
        {
          LOG(LERROR, ("File ugc_types.csv must contain a bit mask of supported ugc traits!"));
          return;
        }

        if (flag)
          maskType |= m_masks[i - 1];
      }

      auto const & typeInfo = line.front();
      std::istringstream iss(typeInfo);
      std::vector<std::string> types{std::istream_iterator<std::string>(iss),
                                     std::istream_iterator<std::string>()};

      m_matcher.AppendType(types, maskType);
    });
  }

public:
  static bool IsUGCAvailable(UGCTypeMask mask) { return mask != UGCTYPE_NONE; }
  static bool IsRatingAvailable(UGCTypeMask mask) { return mask & UGCTYPE_RATING; }
  static bool IsReviewsAvailable(UGCTypeMask mask) { return mask & UGCTYPE_REVIEWS; }
  static bool IsDetailsAvailable(UGCTypeMask mask) { return mask & UGCTYPE_DETAILS; }

  static bool IsUGCAvailable(feature::TypesHolder const & types)
  {
    return IsUGCAvailable(GetValue(types));
  }
  static bool IsRatingAvailable(feature::TypesHolder const & types)
  {
    return IsRatingAvailable(GetValue(types));
  }
  static bool IsReviewsAvailable(feature::TypesHolder const & types)
  {
    return IsReviewsAvailable(GetValue(types));
  }
  static bool IsDetailsAvailable(feature::TypesHolder const & types)
  {
    return IsDetailsAvailable(GetValue(types));
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

class Wheelchair
    : public TraitsBase<Wheelchair, WheelchairAvailability, WheelchairAvailability::No>
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
};

}  // namespace ftraits
