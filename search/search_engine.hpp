#pragma once

#include "params.hpp"
#include "result.hpp"

#include "geometry/rect2d.hpp"

#include "coding/reader.hpp"

#include "base/mutex.hpp"

#include "std/unique_ptr.hpp"
#include "std/string.hpp"
#include "std/function.hpp"
#include "std/atomic.hpp"


class Index;

namespace search
{

class Query;

class EngineData;

class Engine
{
  typedef function<void (Results const &)> SearchCallbackT;
  Results m_searchResults;

public:
  typedef Index IndexType;

  // Doesn't take ownership of @pIndex. Takes ownership of pCategories
  Engine(IndexType const * pIndex, Reader * pCategoriesR,
         ModelReaderPtr polyR, ModelReaderPtr countryR,
         string const & locale);
  ~Engine();

  void SupportOldFormat(bool b);

  void PrepareSearch(m2::RectD const & viewport);
  bool Search(SearchParams const & params, m2::RectD const & viewport);

  void GetResults(Results & res);

  string GetCountryFile(m2::PointD const & pt);
  string GetCountryCode(m2::PointD const & pt);

  int8_t GetCurrentLanguage() const;

private:
  template <class T> string GetCountryNameT(T const & t);
public:
  string GetCountryName(m2::PointD const & pt);
  string GetCountryName(string const & id);

  bool GetNameByType(uint32_t type, int8_t lang, string & name) const;

  m2::RectD GetCountryBounds(string const & file) const;

  void ClearViewportsCache();
  void ClearAllCaches();

private:
  static const int RESULTS_COUNT = 30;

  void SetRankPivot(SearchParams const & params,
                    m2::RectD const & viewport, bool viewportSearch);
  void SetViewportAsync(m2::RectD const & viewport);
  void SearchAsync();

  void EmitResults(SearchParams const & params, Results & res);

  threads::Mutex m_searchMutex, m_updateMutex;
  atomic_flag m_isReadyThread;

  SearchParams m_params;
  m2::RectD m_viewport;

  unique_ptr<search::Query> m_pQuery;
  unique_ptr<EngineData> const m_pData;
};

}  // namespace search
