#pragma once

#include "../indexer/index.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/base.hpp"
#include "../base/string_utils.hpp"

#include "../std/function.hpp"
#include "../std/map.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

class CategoriesHolder;
class Index;

namespace search
{

class Query;
class Result;

class Engine
{
public:
  typedef Index IndexType;

  // Doesn't take ownership of @pIndex. Takes ownership of pCategories
  Engine(IndexType const * pIndex, CategoriesHolder * pCategories);
  ~Engine();

  void SetViewport(m2::RectD const & viewport);
  void Search(string const & query, function<void (Result const &)> const & f);

private:
  Index const * m_pIndex;
  map<strings::UniString, vector<uint32_t> > m_categories;
  scoped_ptr<search::Query> m_pQuery;
};

}  // namespace search
