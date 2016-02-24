#pragma once

#include "params.hpp"
#include "result.hpp"
#include "search_query_factory.hpp"
#include "suggest.hpp"

#include "indexer/categories_holder.hpp"

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
#include "std/vector.hpp"
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
  struct Params
  {
    Params();
    Params(string const & locale, size_t numThreads);

    string m_locale;

    // This field controls number of threads SearchEngine will create
    // to process queries. Use this field wisely as large values may
    // negatively affect performance due to false sharing.
    size_t m_numThreads;
  };

  // Doesn't take ownership of index. Takes ownership of categoriesR.
  Engine(Index & index, CategoriesHolder const & categories,
         storage::CountryInfoGetter const & infoGetter, unique_ptr<SearchQueryFactory> factory,
         Params const & params);
  ~Engine();

  // Posts search request to the queue and returns its handle.
  weak_ptr<QueryHandle> Search(SearchParams const & params, m2::RectD const & viewport);

  // Posts request to support old format to the queue.
  void SetSupportOldFormat(bool support);

  // Posts request to clear caches to the queue.
  void ClearCaches();

private:
  using TTask = function<void(Query & query)>;

  // *ALL* following methods are executed on the m_threads threads.
  void SetRankPivot(SearchParams const & params, m2::RectD const & viewport, bool viewportSearch,
                    Query & query);

  void EmitResults(SearchParams const & params, Results const & res);

  // This method executes tasks from a common pool (|tasks|) in a FIFO
  // manner.  |broadcast| contains per-thread tasks, but nevertheless
  // all necessary synchronization primitives must be used to access
  // |tasks| and |broadcast|.
  void MainLoop(Query & query, queue<TTask> & tasks, queue<TTask> & broadcast);

  void PostTask(TTask && task);

  void PostBroadcast(TTask const & task);

  void DoSearch(SearchParams const & params, m2::RectD const & viewport,
                shared_ptr<QueryHandle> handle, Query & query);

  void DoSupportOldFormat(bool support, Query & query);

  void DoClearCaches(Query & query);

  CategoriesHolder const & m_categories;
  vector<Suggest> m_suggests;

  bool m_shutdown;
  mutex m_mu;
  condition_variable m_cv;

  // List of per-thread pools, used to deliver broadcast messages to
  // search threads.
  vector<queue<TTask>> m_broadcast;

  // Common pool of queries, used to store search tasks.
  queue<TTask> m_tasks;

  vector<unique_ptr<Query>> m_queries;
  vector<threads::SimpleThread> m_threads;
};
}  // namespace search
