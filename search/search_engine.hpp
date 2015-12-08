#pragma once

#include "categories_holder.hpp"
#include "params.hpp"
#include "result.hpp"
#include "search_query_factory.hpp"
#include "suggest.hpp"

#include "geometry/rect2d.hpp"

#include "coding/reader.hpp"

#include "base/macros.hpp"
#include "base/mutex.hpp"
#include "base/thread.hpp"

#include "std/atomic.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/queue.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/weak_ptr.hpp"

class Index;

namespace storage
{
class CountryInfoGetter;
}

namespace search
{
class EngineData;
class Query;

// This class is used as a reference to a search query in the
// SearchEngine's queue.  It's only possible to cancel a search
// request via this reference.
//
// NOTE: this class is thread-safe.
class QueryHandle
{
public:
  QueryHandle();

  // Cancels query this handle points to.
  void Cancel();

private:
  friend class Engine;

  // Attaches the handle to a |query|. If there was or will be a
  // cancel signal, this signal will be propagated to |query|.  This
  // method is called only once, when search engine starts to process
  // query this handle corresponds to.
  void Attach(Query & query);

  // Detaches handle from a query. This method is called only once,
  // when search engine completes process of a query this handle
  // corresponds to.
  void Detach();

  Query * m_query;
  bool m_cancelled;
  mutex m_mu;

  DISALLOW_COPY_AND_MOVE(QueryHandle);
};

// This class is a wrapper around thread which processes search
// queries one by one.
//
// NOTE: this class is thread safe.
class Engine
{
public:
  // Doesn't take ownership of index. Takes ownership of pCategories
  Engine(Index & index, Reader * categoriesR, storage::CountryInfoGetter const & infoGetter,
         string const & locale, unique_ptr<SearchQueryFactory> && factory);
  ~Engine();

  // Posts search request to the queue and returns its handle.
  weak_ptr<QueryHandle> Search(SearchParams const & params, m2::RectD const & viewport);

  // Posts request to support old format to the queue.
  void SetSupportOldFormat(bool support);

  // Posts request to clear caches to the queue.
  void ClearCaches();

  bool GetNameByType(uint32_t type, int8_t lang, string & name) const;

private:
  // *ALL* following methods are executed on the m_loop thread.

  void SetRankPivot(SearchParams const & params, m2::RectD const & viewport, bool viewportSearch);

  void EmitResults(SearchParams const & params, Results const & res);

  // This method executes tasks from |m_tasks| in a FIFO manner.
  void MainLoop();

  void PostTask(function<void()> && task);

  void DoSearch(SearchParams const & params, m2::RectD const & viewport,
                shared_ptr<QueryHandle> handle);

  void DoSupportOldFormat(bool support);

  void DoClearCaches();

  CategoriesHolder m_categories;
  vector<Suggest> m_suggests;

  unique_ptr<Query> m_query;
  unique_ptr<SearchQueryFactory> m_factory;

  bool m_shutdown;
  mutex m_mu;
  condition_variable m_cv;
  queue<function<void()>> m_tasks;
  threads::SimpleThread m_thread;
};
}  // namespace search
