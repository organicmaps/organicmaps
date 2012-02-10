#pragma once

#include "params.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/reader.hpp"

#include "../base/base.hpp"
#include "../base/string_utils.hpp"
#include "../base/mutex.hpp"

#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/function.hpp"


class CategoriesHolder;
class Index;

namespace search
{

struct CategoryInfo;
class Query;
class Results;

class EngineData;

class Engine
{
  typedef function<void (Results const &)> SearchCallbackT;

public:
  typedef Index IndexType;

  // Doesn't take ownership of @pIndex. Takes ownership of pCategories
  Engine(IndexType const * pIndex, CategoriesHolder * pCategories,
         ModelReaderPtr polyR, ModelReaderPtr countryR,
         string const & lang);
  ~Engine();

  void PrepareSearch(m2::RectD const & viewport, bool nearMe,
                     double lat, double lon);
  void Search(SearchParams const & params, m2::RectD const & viewport);

  string GetCountryFile(m2::PointD const & pt) const;

private:
  void InitializeCategoriesAndSuggestStrings(CategoriesHolder const & categories);

  void SetViewportAsync(m2::RectD const & viewport);
  void SearchAsync();

  threads::Mutex m_searchMutex, m_updateMutex;

  SearchParams m_params;
  m2::RectD m_viewport;

  Index const * m_pIndex;
  scoped_ptr<search::Query> m_pQuery;
  scoped_ptr<EngineData> m_pData;
};

}  // namespace search
