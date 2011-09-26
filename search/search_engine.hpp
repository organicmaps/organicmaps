#pragma once

#include "../indexer/index.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/base.hpp"

#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"

class Index;

namespace search
{

class CategoriesHolder;
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
  scoped_ptr<CategoriesHolder> m_pCategories;
  scoped_ptr<search::Query> m_pQuery;
};

}  // namespace search
