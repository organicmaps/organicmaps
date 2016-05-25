#pragma once

#include "search/suggest.hpp"
#include "search/v2/processor_v2.hpp"

#include "std/unique_ptr.hpp"

namespace storage
{
class CountryInfoGetter;
}

namespace search
{
class ProcessorFactory
{
public:
  virtual ~ProcessorFactory() = default;

  virtual unique_ptr<Processor> Build(Index & index, CategoriesHolder const & categories,
                                      vector<Suggest> const & suggests,
                                      storage::CountryInfoGetter const & infoGetter)
  {
    return make_unique<v2::ProcessorV2>(index, categories, suggests, infoGetter);
  }
};
}  // namespace search
