#include "search/engine.hpp"

#include "search/processor.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <map>
#include <vector>

namespace search
{
using namespace std;

namespace
{
class InitSuggestions
{
  map<pair<strings::UniString, int8_t>, uint8_t> m_suggests;

public:
  void operator()(CategoriesHolder::Category::Name const & name)
  {
    if (name.m_prefixLengthToSuggest != CategoriesHolder::Category::kEmptyPrefixLength)
    {
      strings::UniString const uniName = NormalizeAndSimplifyString(name.m_name);

      uint8_t & score = m_suggests[make_pair(uniName, name.m_locale)];
      if (score == 0 || score > name.m_prefixLengthToSuggest)
        score = name.m_prefixLengthToSuggest;
    }
  }

  void GetSuggests(vector<Suggest> & suggests) const
  {
    suggests.reserve(suggests.size() + m_suggests.size());
    for (auto const & s : m_suggests)
      suggests.emplace_back(s.first.first, s.second, s.first.second);
  }
};
}  // namespace

// ProcessorHandle----------------------------------------------------------------------------------
ProcessorHandle::ProcessorHandle() : m_processor(nullptr), m_cancelled(false) {}

void ProcessorHandle::Cancel()
{
  lock_guard<mutex> lock(m_mu);
  m_cancelled = true;
  if (m_processor)
    m_processor->Cancel();
}

void ProcessorHandle::Attach(Processor & processor)
{
  lock_guard<mutex> lock(m_mu);
  m_processor = &processor;
  if (m_cancelled)
    m_processor->Cancel();
}

void ProcessorHandle::Detach()
{
  lock_guard<mutex> lock(m_mu);
  m_processor = nullptr;
}

// Engine::Params ----------------------------------------------------------------------------------
Engine::Params::Params() : m_locale("en"), m_numThreads(1) {}

Engine::Params::Params(string const & locale, size_t numThreads) : m_locale(locale), m_numThreads(numThreads) {}

// Engine ------------------------------------------------------------------------------------------
Engine::Engine(DataSource & dataSource, CategoriesHolder const & categories,
               storage::CountryInfoGetter const & infoGetter, Params const & params)
  : m_shutdown(false)
{
  InitSuggestions doInit;
  categories.ForEachName(doInit);
  doInit.GetSuggests(m_suggests);

  m_contexts.resize(params.m_numThreads);
  for (size_t i = 0; i < params.m_numThreads; ++i)
  {
    auto processor = make_unique<Processor>(dataSource, categories, m_suggests, infoGetter);
    processor->SetPreferredLocale(params.m_locale);
    m_contexts[i].m_processor = std::move(processor);
  }

  m_threads.reserve(params.m_numThreads);
  for (size_t i = 0; i < params.m_numThreads; ++i)
    m_threads.emplace_back(&Engine::MainLoop, this, ref(m_contexts[i]));

  CacheWorldLocalities();
  LoadCitiesBoundaries();
  LoadCountriesTree();
}

Engine::~Engine()
{
  {
    lock_guard<mutex> lock(m_mu);
    m_shutdown = true;
    m_cv.notify_all();
  }

  for (auto & thread : m_threads)
    thread.join();
}

weak_ptr<ProcessorHandle> Engine::Search(SearchParams params)
{
  shared_ptr<ProcessorHandle> handle(new ProcessorHandle());
  PostMessage(Message::TYPE_TASK, [this, params = std::move(params), handle](Processor & processor)
  { DoSearch(std::move(params), handle, processor); });
  return handle;
}

void Engine::SetLocale(string const & locale)
{
  PostMessage(Message::TYPE_BROADCAST, [locale](Processor & processor) { processor.SetPreferredLocale(locale); });
}

size_t Engine::GetNumThreads() const
{
  return m_threads.size();
}

void Engine::ClearCaches()
{
  PostMessage(Message::TYPE_BROADCAST, [](Processor & processor) { processor.ClearCaches(); });
}

void Engine::CacheWorldLocalities()
{
  PostMessage(Message::TYPE_BROADCAST, [](Processor & processor) { processor.CacheWorldLocalities(); });
}

void Engine::LoadCitiesBoundaries()
{
  PostMessage(Message::TYPE_BROADCAST, [](Processor & processor) { processor.LoadCitiesBoundaries(); });
}

void Engine::LoadCountriesTree()
{
  PostMessage(Message::TYPE_BROADCAST, [](Processor & processor) { processor.LoadCountriesTree(); });
}

void Engine::EnableIndexingOfBookmarksDescriptions(bool enable)
{
  PostMessage(Message::TYPE_BROADCAST,
              [enable](Processor & processor) { processor.EnableIndexingOfBookmarksDescriptions(enable); });
}

void Engine::EnableIndexingOfBookmarkGroup(bookmarks::GroupId const & groupId, bool enable)
{
  PostMessage(Message::TYPE_BROADCAST,
              [=](Processor & processor) { processor.EnableIndexingOfBookmarkGroup(groupId, enable); });
}

void Engine::ResetBookmarks()
{
  PostMessage(Message::TYPE_BROADCAST, [](Processor & processor) { processor.ResetBookmarks(); });
}

void Engine::OnBookmarksCreated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks)
{
  PostMessage(Message::TYPE_BROADCAST, [marks](Processor & processor) { processor.OnBookmarksCreated(marks); });
}

void Engine::OnBookmarksUpdated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks)
{
  PostMessage(Message::TYPE_BROADCAST, [marks](Processor & processor) { processor.OnBookmarksUpdated(marks); });
}

void Engine::OnBookmarksDeleted(vector<bookmarks::Id> const & marks)
{
  PostMessage(Message::TYPE_BROADCAST, [marks](Processor & processor) { processor.OnBookmarksDeleted(marks); });
}

void Engine::OnBookmarksAttachedToGroup(bookmarks::GroupId const & groupId, vector<bookmarks::Id> const & marks)
{
  PostMessage(Message::TYPE_BROADCAST,
              [groupId, marks](Processor & processor) { processor.OnBookmarksAttachedToGroup(groupId, marks); });
}

void Engine::OnBookmarksDetachedFromGroup(bookmarks::GroupId const & groupId, vector<bookmarks::Id> const & marks)
{
  PostMessage(Message::TYPE_BROADCAST,
              [groupId, marks](Processor & processor) { processor.OnBookmarksDetachedFromGroup(groupId, marks); });
}

void Engine::MainLoop(Context & context)
{
  while (true)
  {
    bool hasBroadcast = false;
    queue<Message> messages;

    {
      unique_lock<mutex> lock(m_mu);
      m_cv.wait(lock, [&]() { return m_shutdown || !m_messages.empty() || !context.m_messages.empty(); });

      if (m_shutdown)
        break;

      // As SearchEngine is thread-safe, there is a global order on
      // public API requests, and this order is kept by the global
      // |m_messages| queue.  When a broadcast message arrives, it
      // must be executed in any case by all threads, therefore the
      // first free thread extracts as many as possible broadcast
      // messages from |m_messages| front and replicates them to all
      // thread-specific |m_messages| queues.
      while (!m_messages.empty() && m_messages.front().m_type == Message::TYPE_BROADCAST)
      {
        for (auto & b : m_contexts)
          b.m_messages.push(m_messages.front());
        m_messages.pop();
        hasBroadcast = true;
      }

      // Consumes first non-broadcast message, if any.  We process
      // only a single task message (in constrast with broadcast
      // messages) because task messages are actually search queries,
      // whose processing may take an arbitrary amount of time. So
      // it's better to process only one message and leave rest to the
      // next free search thread.
      if (!m_messages.empty())
      {
        context.m_messages.push(std::move(m_messages.front()));
        m_messages.pop();
      }

      messages.swap(context.m_messages);
    }

    if (hasBroadcast)
      m_cv.notify_all();

    while (!messages.empty())
    {
      messages.front()(*context.m_processor);
      messages.pop();
    }
  }
}

template <typename... Args>
void Engine::PostMessage(Args &&... args)
{
  lock_guard<mutex> lock(m_mu);
  m_messages.emplace(std::forward<Args>(args)...);
  m_cv.notify_one();
}

void Engine::DoSearch(SearchParams params, shared_ptr<ProcessorHandle> handle, Processor & processor)
{
  LOG(LINFO, ("Search started:", params.m_mode, params.m_viewport));
  base::Timer timer;
  SCOPE_GUARD(printDuration, [&timer]() { LOG(LINFO, ("Search ended in", timer.ElapsedMilliseconds(), "ms.")); });

  processor.Reset();
  handle->Attach(processor);
  SCOPE_GUARD(detach, [&handle] { handle->Detach(); });

  processor.Search(std::move(params));
}
}  // namespace search
