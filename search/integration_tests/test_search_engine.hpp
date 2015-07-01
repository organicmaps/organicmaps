#pragma once

#include "indexer/index.hpp"

#include "geometry/rect2d.hpp"

#include "search/search_engine.hpp"

#include "std/string.hpp"

class Platform;

namespace search
{
class SearchParams;
}

class TestSearchEngine : public Index
{
public:
  TestSearchEngine(std::string const & locale);

  bool Search(search::SearchParams const & params, m2::RectD const & viewport);

private:
  Platform & m_platform;
  search::Engine m_engine;
};
