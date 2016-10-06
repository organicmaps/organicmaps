#include "search/categories_cache.hpp"

#include "search/mwm_context.hpp"
#include "search/query_params.hpp"
#include "search/retrieval.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"

#include "std/vector.hpp"

namespace search
{
namespace
{
// Performs pairwise union of adjacent bit vectors
// until at most one bit vector is left.
void UniteCBVs(vector<CBV> & cbvs)
{
  while (cbvs.size() > 1)
  {
    size_t i = 0;
    size_t j = 0;
    for (; j + 1 < cbvs.size(); j += 2)
      cbvs[i++] = cbvs[j].Union(cbvs[j + 1]);
    for (; j < cbvs.size(); ++j)
      cbvs[i++] = move(cbvs[j]);
    cbvs.resize(i);
  }
}
}  // namespace

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

  QueryParams params;
  params.m_tokens.resize(1);
  params.m_tokens[0].resize(1);

  params.m_types.resize(1);
  params.m_types[0].resize(1);

  vector<CBV> cbvs;

  m_categories.ForEach([&](strings::UniString const & key, uint32_t const type) {
    params.m_tokens[0][0] = key;
    params.m_types[0][0] = type;

    CBV cbv(RetrieveAddressFeatures(context, m_cancellable, params));
    if (!cbv.IsEmpty())
      cbvs.push_back(move(cbv));
  });

  UniteCBVs(cbvs);
  if (cbvs.empty())
    cbvs.emplace_back();

  return cbvs[0];
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
