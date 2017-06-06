#pragma once

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/ftypes_mapping.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace wheelchair
{
enum class Type
{
  No,
  Yes,
  Limited
};

inline std::string DebugPrint(Type wheelchair)
{
  switch (wheelchair)
  {
  case Type::No: return "No";
  case Type::Yes: return "Yes";
  case Type::Limited: return "Limited";
  }
}

class Matcher
{
public:
  static Type GetType(feature::TypesHolder const & types)
  {
    static Matcher instance;
    auto const it = instance.m_matcher.Find(types);
    if (!instance.m_matcher.IsValid(it))
      return Type::No;

    return it->second;
  }

private:
  using TypesInitializer = std::initializer_list<std::initializer_list<char const *>>;

  Matcher()
  {
    m_matcher.Append<TypesInitializer>({{"wheelchair", "no"}}, Type::No);
    m_matcher.Append<TypesInitializer>({{"wheelchair", "yes"}}, Type::Yes);
    m_matcher.Append<TypesInitializer>({{"wheelchair", "limited"}}, Type::Limited);
  }

  ftypes::HashMapMatcher<uint32_t, Type> m_matcher;
};
}  // namespace wheelchair
