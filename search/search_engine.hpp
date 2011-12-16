#pragma once

#include "../indexer/index.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/base.hpp"
#include "../base/string_utils.hpp"

#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"

class CategoriesHolder;
class Index;

namespace search
{

struct CategoryInfo;
class Query;
class Result;

class EngineData;

class Engine
{
public:
  typedef Index IndexType;

  // Doesn't take ownership of @pIndex. Takes ownership of pCategories
  Engine(IndexType const * pIndex, CategoriesHolder * pCategories,
         ModelReaderPtr polyR, ModelReaderPtr countryR);
  ~Engine();

  void SetViewport(m2::RectD const & viewport);
  void SetPreferredLanguage(string const & lang);
  void Search(string const & query, function<void (Result const &)> const & f);

  string GetCountryFile(m2::PointD const & pt) const;

private:
  void InitializeCategoriesAndSuggestStrings(CategoriesHolder const & categories);

  Index const * m_pIndex;
  scoped_ptr<search::Query> m_pQuery;
  scoped_ptr<EngineData> m_pData;
};

}  // namespace search
