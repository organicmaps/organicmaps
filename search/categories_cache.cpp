#include "search/categories_cache.hpp"

#include "search/mwm_context.hpp"
#include "search/query_params.hpp"
#include "search/retrieval.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/levenshtein_dfa.hpp"

#include "std/vector.hpp"

namespace search
{
// CategoriesCache ---------------------------------------------------------------------------------
CBV CategoriesCache::Get(MwmContext const & context)
{
  if (!context.m_handle.IsAlive() || !context.m_value.HasSearchIndex())
    return CBV();

  auto id = context.m_handle.GetId();
  auto const it = m_cache.find(id);
  if (it != m_cache.cend())
    return it->second;

  auto cbv = Load(context);
  m_cache[id] = cbv;
  return cbv;
}

CBV CategoriesCache::Load(MwmContext const & context)
{
  ASSERT(context.m_handle.IsAlive(), ());
  ASSERT(context.m_value.HasSearchIndex(), ());

  auto const & c = classif();

  SearchTrieRequest<strings::LevenshteinDFA> request;

  m_categories.ForEach([&request, &c](uint32_t const type) {
    request.m_categories.emplace_back(FeatureTypeToString(c.GetIndexForType(type)));
  });

  Retrieval retrieval(context, m_cancellable);
  return CBV(retrieval.RetrieveAddressFeatures(request));
}

// StreetsCache ------------------------------------------------------------------------------------
StreetsCache::StreetsCache(my::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsStreetChecker::Instance(), cancellable)
{
}

// VillagesCache -----------------------------------------------------------------------------------
VillagesCache::VillagesCache(my::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsVillageChecker::Instance(), cancellable)
{
}

// HotelsCache -------------------------------------------------------------------------------------
HotelsCache::HotelsCache(my::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsHotelChecker::Instance(), cancellable)
{
}
}  // namespace search
