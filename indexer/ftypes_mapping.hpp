#pragma once

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ftypes
{
template <typename Container>
class Matcher
{
public:
  using ConstIterator = typename Container::const_iterator;

  ConstIterator Find(feature::TypesHolder const & types) const
  {
    for (auto const t : types)
    {
      for (auto level = ftype::GetLevel(t); level; --level)
      {
        auto truncatedType = t;
        ftype::TruncValue(truncatedType, level);
        auto const it = m_mapping.find(truncatedType);

        if (it != m_mapping.cend())
          return it;
      }
    }

    return m_mapping.cend();
  }

  bool IsValid(ConstIterator it) const
  {
    return it != m_mapping.cend();
  }

  bool Contains(feature::TypesHolder const & types) const
  {
    return IsValid(Find(types));
  }

  template<typename TypesPaths, typename ... Args>
  void Append(TypesPaths const & types, Args const & ... args)
  {
    for (auto const & type : types)
    {
#if defined(DEBUG)
      feature::TypesHolder holder;
      holder.Assign(classif().GetTypeByPath(type));
      ASSERT(Find(holder) == m_mapping.cend(), ("This type already exists", type));
#endif
      m_mapping.emplace(classif().GetTypeByPath(type), args...);
    }
  }

private:
  Container m_mapping;
};

template <typename Key, typename Value>
using HashMapMatcher = Matcher<std::unordered_map<Key, Value>>;

template <typename Key>
using HashSetMatcher = Matcher<std::unordered_set<Key>>;
}  // namespace ftypes
