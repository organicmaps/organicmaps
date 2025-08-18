#pragma once

#include "search/categories_set.hpp"
#include "search/cbv.hpp"

#include "indexer/mwm_set.hpp"

#include "base/cancellable.hpp"

#include <map>
#include <vector>

namespace search
{
class MwmContext;

class CategoriesCache
{
public:
  template <typename TypesSource>
  CategoriesCache(TypesSource const & source, base::Cancellable const & cancellable) : m_cancellable(cancellable)
  {
    source.ForEachType([this](uint32_t type) { m_categories.Add(type); });
  }

  CategoriesCache(std::vector<uint32_t> const & types, base::Cancellable const & cancellable)
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
  base::Cancellable const & m_cancellable;
  std::map<MwmSet::MwmId, CBV> m_cache;
};

class StreetsCache : public CategoriesCache
{
public:
  StreetsCache(base::Cancellable const & cancellable);
};

class SuburbsCache : public CategoriesCache
{
public:
  SuburbsCache(base::Cancellable const & cancellable);
};

class VillagesCache : public CategoriesCache
{
public:
  VillagesCache(base::Cancellable const & cancellable);
};

class CountriesCache : public CategoriesCache
{
public:
  CountriesCache(base::Cancellable const & cancellable);
};

class StatesCache : public CategoriesCache
{
public:
  StatesCache(base::Cancellable const & cancellable);
};

// Used for cities/towns/villages from world. Currently we do not have villages in World.mwm but
// it may be good to put some important villages to it: mountain/beach resorts.
class CitiesTownsOrVillagesCache : public CategoriesCache
{
public:
  CitiesTownsOrVillagesCache(base::Cancellable const & cancellable);
};

class HotelsCache : public CategoriesCache
{
public:
  HotelsCache(base::Cancellable const & cancellable);
};

class FoodCache : public CategoriesCache
{
public:
  FoodCache(base::Cancellable const & cancellable);
};
}  // namespace search
