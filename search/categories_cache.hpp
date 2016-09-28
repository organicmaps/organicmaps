#pragma once

#include "search/categories_set.hpp"
#include "search/cbv.hpp"

#include "indexer/mwm_set.hpp"

#include "base/cancellable.hpp"

#include "std/map.hpp"

namespace search
{
class MwmContext;

class CategoriesCache
{
public:
  CategoriesCache(CategoriesSet const & categories, my::Cancellable const & cancellable);

  CBV Get(MwmContext const & context);
  inline void Clear() { m_cache.clear(); }

private:
  CBV Load(MwmContext const & context);

  CategoriesSet const & m_categories;
  my::Cancellable const & m_cancellable;
  map<MwmSet::MwmId, CBV> m_cache;
};

class StreetsCache
{
 public:
  StreetsCache(my::Cancellable const & cancellable);

  inline CBV Get(MwmContext const & context) { return m_cache.Get(context); }
  inline void Clear() { m_cache.Clear(); }
  inline bool HasCategory(strings::UniString const & category) const
  {
    return m_categories.HasKey(category);
  }

 private:
  CategoriesSet m_categories;
  CategoriesCache m_cache;
};

class VillagesCache
{
 public:
  VillagesCache(my::Cancellable const & cancellable);

  inline CBV Get(MwmContext const & context) { return m_cache.Get(context); }
  inline void Clear() { m_cache.Clear(); }

private:
  CategoriesSet m_categories;
  CategoriesCache m_cache;
};
}  // namespace search
