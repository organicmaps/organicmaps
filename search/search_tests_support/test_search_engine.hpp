#pragma once

#include "indexer/index.hpp"

#include "geometry/rect2d.hpp"

#include "search/search_engine.hpp"

#include "storage/country_info_getter.hpp"

#include "std/string.hpp"
#include "std/weak_ptr.hpp"

class Platform;

namespace search
{
class SearchParams;

namespace tests_support
{
class TestSearchEngine : public Index
{
public:
  TestSearchEngine(string const & locale);

  weak_ptr<search::QueryHandle> Search(search::SearchParams const & params,
                                       m2::RectD const & viewport);

private:
  Platform & m_platform;
  storage::CountryInfoGetter m_infoGetter;
  search::Engine m_engine;
};
}  // namespace tests_support
}  // namespace search
