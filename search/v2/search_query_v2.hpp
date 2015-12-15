#pragma once

#include "search/search_query.hpp"
#include "search/v2/geocoder.hpp"

namespace search
{
namespace v2
{
class SearchQueryV2 : public Query
{
public:
  SearchQueryV2(Index & index, CategoriesHolder const & categories,
                vector<Suggest> const & suggests, storage::CountryInfoGetter const & infoGetter);

  // my::Cancellable overrides:
  void Reset() override;
  void Cancel() override;

  // Query overrides:
  void Search(Results & res, size_t resCount) override;
  void SearchViewportPoints(Results & res) override;
  void ClearCaches() override;

protected:
  // Adds a bunch of features as PreResult1.
  void AddPreResults1(vector<FeatureID> & results);

  Geocoder m_geocoder;
};
}  // namespace v2
}  // namespace search
