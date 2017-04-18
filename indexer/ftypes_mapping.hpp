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
  typename Container::const_iterator
  Find(feature::TypesHolder const & types) const
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

  bool IsValid(typename Container::const_iterator it) const
  {
    return it != m_mapping.cend();
  }

  bool Contains(feature::TypesHolder const & types) const
  {
    return IsValid(Find(types));
  }

  template<typename ... Args>
  void Append(std::vector<std::vector<std::string>> const & types, Args ... args)
  {
    for (auto const & type : types)
    {
#if defined(DEBUG)
      feature::TypesHolder holder;
      holder.Assign(classif().GetTypeByPath(type));
      ASSERT(Find(holder) == m_mapping.cend(), ("This type already exists", type));
#endif
      m_mapping.emplace(classif().GetTypeByPath(type), std::forward<Args>(args)...);
    }
  }

private:
  Container m_mapping;
};

template <typename Key, typename Value>
using HashMap = Matcher<std::unordered_map<Key, Value>>;

template <typename Key>
using HashSet = Matcher<std::unordered_set<Key>>;
}  // namespace ftypes
