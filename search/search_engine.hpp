#pragma once

#include "params.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/reader.hpp"

#include "../base/mutex.hpp"

#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/function.hpp"


class Index;

namespace search
{

class Query;
class Results;

class EngineData;

class Engine
{
  typedef function<void (Results const &)> SearchCallbackT;

public:
  typedef Index IndexType;

  // Doesn't take ownership of @pIndex. Takes ownership of pCategories
  Engine(IndexType const * pIndex, Reader * pCategoriesR,
         ModelReaderPtr polyR, ModelReaderPtr countryR,
         string const & lang);
  ~Engine();

  void PrepareSearch(m2::RectD const & viewport,
                     bool hasPt, double lat, double lon);
  void Search(SearchParams const & params, m2::RectD const & viewport);

  string GetCountryFile(m2::PointD const & pt);
  string GetCountryCode(m2::PointD const & pt);
  string GetCountryName(m2::PointD const & pt);

  void ClearCaches();

private:
  static const int RESULTS_COUNT = 15;

  void SetViewportAsync(m2::RectD const & viewport, m2::RectD const & nearby);
  void SearchAsync();

  threads::Mutex m_searchMutex, m_updateMutex, m_readyMutex;

  volatile bool m_readyThread;

  SearchParams m_params;
  m2::RectD m_viewport;

  Index const * m_pIndex;
  scoped_ptr<search::Query> m_pQuery;
  scoped_ptr<EngineData> m_pData;
};

}  // namespace search
