#pragma once

#include "search/search_params.hpp"
#include "search/suggest.hpp"

#include "indexer/categories_holder.hpp"

#include "base/macros.hpp"
#include "base/thread.hpp"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

class DataSource;

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
  std::mutex m_mu;

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
    Params(std::string const & locale, size_t numThreads);

    std::string m_locale;

    // This field controls number of threads SearchEngine will create
    // to process queries. Use this field wisely as large values may
    // negatively affect performance due to false sharing.
    size_t m_numThreads;
  };

  // Doesn't take ownership of dataSource and categories.
  Engine(DataSource & dataSource, CategoriesHolder const & categories, storage::CountryInfoGetter const & infoGetter,
         Params const & params);
  ~Engine();

  // Posts search request to the queue and returns its handle.
  std::weak_ptr<ProcessorHandle> Search(SearchParams params);

  // Sets default locale on all query processors.
  void SetLocale(std::string const & locale);

  // Returns the number of request-processing threads.
  size_t GetNumThreads() const;

  // Posts request to clear caches to the queue.
  void ClearCaches();

  // Posts requests to load and cache localities from World.mwm.
  void CacheWorldLocalities();

  // Posts request to reload cities boundaries tables.
  void LoadCitiesBoundaries();

  // Posts request to load countries tree.
  void LoadCountriesTree();

  void EnableIndexingOfBookmarksDescriptions(bool enable);
  void EnableIndexingOfBookmarkGroup(bookmarks::GroupId const & groupId, bool enable);

  // Clears all bookmarks data and caches for all processors.
  void ResetBookmarks();

  void OnBookmarksCreated(std::vector<std::pair<bookmarks::Id, bookmarks::Doc>> const & marks);
  void OnBookmarksUpdated(std::vector<std::pair<bookmarks::Id, bookmarks::Doc>> const & marks);
  void OnBookmarksDeleted(std::vector<bookmarks::Id> const & marks);
  void OnBookmarksAttachedToGroup(bookmarks::GroupId const & groupId, std::vector<bookmarks::Id> const & marks);
  void OnBookmarksDetachedFromGroup(bookmarks::GroupId const & groupId, std::vector<bookmarks::Id> const & marks);

private:
  struct Message
  {
    using Fn = std::function<void(Processor & processor)>;

    enum Type
    {
      TYPE_TASK,
      TYPE_BROADCAST
    };

    template <typename Gn>
    Message(Type type, Gn && gn) : m_type(type)
                                 , m_fn(std::forward<Gn>(gn))
    {}

    void operator()(Processor & processor) { m_fn(processor); }

    Type m_type;
    Fn m_fn;
  };

  struct Context
  {
    // This field *CAN* be accessed by other threads, so |m_mu| must
    // be taken before access this queue.  Messages are ordered here
    // by a timestamp and all timestamps are less than timestamps in
    // the global |m_messages| queue.
    std::queue<Message> m_messages;

    // This field is thread-specific and *CAN NOT* be accessed by
    // other threads.
    std::unique_ptr<Processor> m_processor;
  };

  // *ALL* following methods are executed on the m_threads threads.

  // This method executes tasks from a common pool (|tasks|) in a FIFO
  // manner.  |broadcast| contains per-thread tasks, but nevertheless
  // all necessary synchronization primitives must be used to access
  // |tasks| and |broadcast|.
  void MainLoop(Context & context);

  template <typename... Args>
  void PostMessage(Args &&... args);

  void DoSearch(SearchParams params, std::shared_ptr<ProcessorHandle> handle, Processor & processor);

  std::vector<Suggest> m_suggests;

  bool m_shutdown;
  std::mutex m_mu;
  std::condition_variable m_cv;

  std::queue<Message> m_messages;
  std::vector<Context> m_contexts;
  std::vector<threads::SimpleThread> m_threads;
};
}  // namespace search
