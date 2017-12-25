#pragma once

#include "search/bookmarks/processor.hpp"
#include "search/result.hpp"
#include "search/search_params.hpp"
#include "search/suggest.hpp"

#include "indexer/categories_holder.hpp"

#include "coding/reader.hpp"

#include "base/macros.hpp"
#include "base/mutex.hpp"
#include "base/thread.hpp"

#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/queue.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
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
class Processor;

// This class is used as a reference to a search processor in the
// SearchEngine's queue.  It's only possible to cancel a search
// request via this reference.
//
// NOTE: this class is thread-safe.
class ProcessorHandle
{
public:
  ProcessorHandle();

  // Cancels processor this handle points to.
  void Cancel();

private:
  friend class Engine;

  // Attaches the handle to a |processor|. If there was or will be a
  // cancel signal, this signal will be propagated to |processor|.
  // This method is called only once, when search engine starts
  // the processor this handle corresponds to.
  void Attach(Processor & processor);

  // Detaches handle from a processor. This method is called only
  // once, when search engine completes processing of the query
  // that this handle corresponds to.
  void Detach();

  Processor * m_processor;
  bool m_cancelled;
  mutex m_mu;

  DISALLOW_COPY_AND_MOVE(ProcessorHandle);
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

  // Doesn't take ownership of index and categories.
  Engine(Index & index, CategoriesHolder const & categories,
         storage::CountryInfoGetter const & infoGetter, Params const & params);
  ~Engine();

  // Posts search request to the queue and returns its handle.
  weak_ptr<ProcessorHandle> Search(SearchParams const & params);

  // Sets default locale on all query processors.
  void SetLocale(string const & locale);

  // Posts request to clear caches to the queue.
  void ClearCaches();

  // Posts request to reload cities boundaries tables.
  void LoadCitiesBoundaries();

  void OnBookmarksCreated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks);
  void OnBookmarksUpdated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks);
  void OnBookmarksDeleted(vector<bookmarks::Id> const & marks);

private:
  struct Message
  {
    using Fn = function<void(Processor & processor)>;

    enum Type
    {
      TYPE_TASK,
      TYPE_BROADCAST
    };

    template <typename Gn>
    Message(Type type, Gn && gn) : m_type(type), m_fn(forward<Gn>(gn)) {}

    void operator()(Processor & processor) { m_fn(processor); }

    Type m_type;
    Fn m_fn;
  };

  // alignas() is used here to prevent false-sharing between different
  // threads.
  struct alignas(64 /* the most common cache-line size */) Context
  {
    // This field *CAN* be accessed by other threads, so |m_mu| must
    // be taken before access this queue.  Messages are ordered here
    // by a timestamp and all timestamps are less than timestamps in
    // the global |m_messages| queue.
    queue<Message> m_messages;

    // This field is thread-specific and *CAN NOT* be accessed by
    // other threads.
    unique_ptr<Processor> m_processor;
  };

  // *ALL* following methods are executed on the m_threads threads.

  // This method executes tasks from a common pool (|tasks|) in a FIFO
  // manner.  |broadcast| contains per-thread tasks, but nevertheless
  // all necessary synchronization primitives must be used to access
  // |tasks| and |broadcast|.
  void MainLoop(Context & context);

  template <typename... TArgs>
  void PostMessage(TArgs &&... args);

  void DoSearch(SearchParams const & params, shared_ptr<ProcessorHandle> handle,
                Processor & processor);

  vector<Suggest> m_suggests;

  bool m_shutdown;
  mutex m_mu;
  condition_variable m_cv;

  queue<Message> m_messages;
  vector<Context> m_contexts;
  vector<threads::SimpleThread> m_threads;
};
}  // namespace search
