#pragma once

#include "search/categories_set.hpp"
#include "search/cbv.hpp"

#include "indexer/mwm_set.hpp"

#include "base/cancellable.hpp"

#include "std/map.hpp"
#include "std/set.hpp"

namespace search
{
class MwmContext;

class CategoriesCache
{
public:
  template <typename TypesSource>
  CategoriesCache(TypesSource const & source, my::Cancellable const & cancellable)
    : m_cancellable(cancellable)
  {
    source.ForEachType([this](uint32_t type) { m_categories.Add(type); });
  }

  CategoriesCache(set<uint32_t> const & types, my::Cancellable const & cancellable)
    : m_cancellable(cancellable)
  {
    for (uint32_t type : types)
      m_categories.Add(type);
  }

  virtual ~CategoriesCache() = default;

  CBV Get(MwmContext const & context);

  inline void Clear() { m_cache.clear(); }

private:
  CBV Load(MwmContext const & context) const;

  CategoriesSet m_categories;
  my::Cancellable const & m_cancellable;
  map<MwmSet::MwmId, CBV> m_cache;
};

class StreetsCache : public CategoriesCache
{
public:
  StreetsCache(my::Cancellable const & cancellable);
};

class VillagesCache : public CategoriesCache
{
public:
  VillagesCache(my::Cancellable const & cancellable);
};

class HotelsCache : public CategoriesCache
{
public:
  HotelsCache(my::Cancellable const & cancellable);
};
}  // namespace search
