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
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace ftraits
{
template <typename Base, typename Value>
class TraitsBase
{
public:
  static std::optional<Value> GetValue(feature::TypesHolder const & types)
  {
    auto const & instance = Instance();
    auto const it = Find(types);
    if (!instance.m_matcher.IsValid(it))
      return std::nullopt;

    return it->second;
  }

  static std::optional<uint32_t> GetType(feature::TypesHolder const & types)
  {
    auto const & instance = Instance();
    auto const it = Find(types);
    if (!instance.m_matcher.IsValid(it))
      return std::nullopt;

    return it->first;
  }

private:
  using ConstIterator = typename ftypes::HashMapMatcher<uint32_t, Value>::ConstIterator;

  static ConstIterator Find(feature::TypesHolder const & types)
  {
    auto const & instance = Instance();

    auto const excluded = instance.m_excluded.Find(types);
    if (instance.m_excluded.IsValid(excluded))
      return instance.m_matcher.End();

    return instance.m_matcher.Find(types);
  }

protected:
  static TraitsBase const & Instance()
  {
    static Base instance;
    return instance;
  }

  ftypes::HashMapMatcher<uint32_t, Value> m_matcher;
  ftypes::HashSetMatcher<uint32_t> m_excluded;
};

enum class WheelchairAvailability
{
  No,
  Yes,
  Limited,
};

inline std::string DebugPrint(WheelchairAvailability wheelchair)
{
  switch (wheelchair)
  {
  case WheelchairAvailability::No: return "No";
  case WheelchairAvailability::Yes: return "Yes";
  case WheelchairAvailability::Limited: return "Limited";
  }
  UNREACHABLE();
}

class Wheelchair : public TraitsBase<Wheelchair, WheelchairAvailability>
{
  friend TraitsBase;

  using TypesInitializer = std::initializer_list<std::initializer_list<char const *>>;

  Wheelchair()
  {
    m_matcher.Append<TypesInitializer>({{"wheelchair", "no"}}, WheelchairAvailability::No);
    m_matcher.Append<TypesInitializer>({{"wheelchair", "yes"}}, WheelchairAvailability::Yes);
    m_matcher.Append<TypesInitializer>({{"wheelchair", "limited"}}, WheelchairAvailability::Limited);
  }
};

enum class DrinkingWaterAvailability
{
  No,
  Yes,
};

class DrinkingWater : public TraitsBase<DrinkingWater, DrinkingWaterAvailability>
{
  friend TraitsBase;

  using TypesInitializer = std::initializer_list<std::initializer_list<char const *>>;

  DrinkingWater()
  {
    m_matcher.Append<TypesInitializer>({{"drinking_water", "no"}}, DrinkingWaterAvailability::No);
    m_matcher.Append<TypesInitializer>({{"drinking_water", "yes"}}, DrinkingWaterAvailability::Yes);
    m_matcher.Append<TypesInitializer>({{"amenity", "drinking_water"}}, DrinkingWaterAvailability::Yes);
  }
};

}  // namespace ftraits
