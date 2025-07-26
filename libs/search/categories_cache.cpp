#include "search/categories_cache.hpp"

#include "search/mwm_context.hpp"
#include "search/retrieval.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_string_utils.hpp"

namespace search
{
using namespace std;

// CategoriesCache ---------------------------------------------------------------------------------
CBV CategoriesCache::Get(MwmContext const & context)
{
  auto const id = context.m_handle.GetId();
  auto const it = m_cache.find(id);
  if (it != m_cache.cend())
    return it->second;

  auto cbv = Load(context);
  m_cache[id] = cbv;
  return cbv;
}

CBV CategoriesCache::Load(MwmContext const & context) const
{
  auto const & c = classif();

  // Any DFA will do, since we only use requests's m_categories,
  // but the interface of Retrieval forces us to make a choice.
  SearchTrieRequest<strings::UniStringDFA> request;

  // m_categories usually has truncated types; add them together with their subtrees.
  m_categories.ForEach([&request, &c](uint32_t const type)
  {
    c.ForEachInSubtree([&](uint32_t descendantType)
    { request.m_categories.emplace_back(FeatureTypeToString(c.GetIndexForType(descendantType))); }, type);
  });

  Retrieval retrieval(context, m_cancellable);
  return retrieval.RetrieveAddressFeatures(request).m_features;
}

// StreetsCache ------------------------------------------------------------------------------------
StreetsCache::StreetsCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsStreetOrSquareChecker::Instance(), cancellable)
{}

// SuburbsCache ------------------------------------------------------------------------------------
SuburbsCache::SuburbsCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsSuburbChecker::Instance(), cancellable)
{}
// VillagesCache -----------------------------------------------------------------------------------
VillagesCache::VillagesCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsVillageChecker::Instance(), cancellable)
{}

// CountriesCache ----------------------------------------------------------------------------------
CountriesCache::CountriesCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsCountryChecker::Instance(), cancellable)
{}

// StatesCache -------------------------------------------------------------------------------------
StatesCache::StatesCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsStateChecker::Instance(), cancellable)
{}

// CitiesTownsOrVillagesCache ----------------------------------------------------------------------
CitiesTownsOrVillagesCache::CitiesTownsOrVillagesCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsCityTownOrVillageChecker::Instance(), cancellable)
{}

// HotelsCache -------------------------------------------------------------------------------------
HotelsCache::HotelsCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsHotelChecker::Instance(), cancellable)
{}

// FoodCache ---------------------------------------------------------------------------------------
FoodCache::FoodCache(base::Cancellable const & cancellable)
  : CategoriesCache(ftypes::IsEatChecker::Instance(), cancellable)
{}
}  // namespace search
