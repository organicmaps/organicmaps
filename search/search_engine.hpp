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
class Result;

class Engine
{
public:
  typedef Index IndexType;

  // Doesn't take ownership of @pIndex. Takes ownership of pCategories
  Engine(IndexType const * pIndex, CategoriesHolder * pCategories);
  ~Engine();

  void Search(string const & query,
              m2::RectD const & rect,
              function<void (Result const &)> const & f);

private:
  Index const * m_pIndex;
  scoped_ptr<CategoriesHolder> m_pCategories;
};

}  // namespace search
