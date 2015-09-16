#pragma once

#include "params.hpp"
#include "result.hpp"
#include "search_query_factory.hpp"

#include "geometry/rect2d.hpp"

#include "coding/reader.hpp"

#include "base/mutex.hpp"

#include "std/atomic.hpp"
#include "std/function.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"

class Index;

namespace storage
{
class CountryInfoGetter;
}

namespace search
{
class EngineData;
class Query;

class Engine
{
  typedef function<void (Results const &)> SearchCallbackT;

public:
  // Doesn't take ownership of index. Takes ownership of pCategories
  Engine(Index & index, Reader * categoriesR, storage::CountryInfoGetter const & infoGetter,
         string const & locale, unique_ptr<SearchQueryFactory> && factory);
  ~Engine();

  void SupportOldFormat(bool b);

  void PrepareSearch(m2::RectD const & viewport);
  bool Search(SearchParams const & params, m2::RectD const & viewport);

  int8_t GetCurrentLanguage() const;

  bool GetNameByType(uint32_t type, int8_t lang, string & name) const;

  void ClearViewportsCache();
  void ClearAllCaches();

private:
  static const int RESULTS_COUNT = 30;

  void SetRankPivot(SearchParams const & params,
                    m2::RectD const & viewport, bool viewportSearch);
  void SetViewportAsync(m2::RectD const & viewport);
  void SearchAsync();

  void EmitResults(SearchParams const & params, Results const & res);

  threads::Mutex m_searchMutex;
  threads::Mutex m_updateMutex;
  atomic_flag m_isReadyThread;

  SearchParams m_params;
  m2::RectD m_viewport;

  unique_ptr<Query> m_query;
  unique_ptr<SearchQueryFactory> m_factory;
  unique_ptr<EngineData> const m_data;
};

}  // namespace search
