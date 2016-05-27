#pragma once

#include "search/suggest.hpp"
#include "search/processor.hpp"

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
    return make_unique<Processor>(index, categories, suggests, infoGetter);
  }
};
}  // namespace search
