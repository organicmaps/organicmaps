#pragma once

#include "search/processor.hpp"
#include "search/v2/geocoder.hpp"

namespace search
{
namespace v2
{
class ProcessorV2 : public Processor
{
public:
  ProcessorV2(Index & index, CategoriesHolder const & categories, vector<Suggest> const & suggests,
              storage::CountryInfoGetter const & infoGetter);

  // my::Cancellable overrides:
  void Reset() override;
  void Cancel() override;

  // Query overrides:
  void Search(Results & results, size_t limit) override;
  void SearchViewportPoints(Results & results) override;
  void ClearCaches() override;

protected:
  Geocoder m_geocoder;
};
}  // namespace v2
}  // namespace search
