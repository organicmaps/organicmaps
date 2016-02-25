#include "search_engine.hpp"

#include "geometry_utils.hpp"
#include "search_query.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/map.hpp"
#include "std/vector.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace search
{
namespace
{
int const kResultsCount = 30;

class InitSuggestions
{
  using TSuggestMap = map<pair<strings::UniString, int8_t>, uint8_t>;
  TSuggestMap m_suggests;

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

void SendStatistics(SearchParams const & params, m2::RectD const & viewport, Results const & res)
{
  size_t const kMaxNumResultsToSend = 10;

  size_t const numResultsToSend = min(kMaxNumResultsToSend, res.GetCount());
  string resultString = strings::to_string(numResultsToSend);
  for (size_t i = 0; i < numResultsToSend; ++i)
    resultString.append("\t" + res.GetResult(i).ToStringForStats());

  string posX, posY;
  if (params.IsValidPosition())
  {
    posX = strings::to_string(MercatorBounds::LonToX(params.m_lon));
    posY = strings::to_string(MercatorBounds::LatToY(params.m_lat));
  }

  alohalytics::TStringMap const stats = {
      {"posX", posX},
      {"posY", posY},
      {"viewportMinX", strings::to_string(viewport.minX())},
      {"viewportMinY", strings::to_string(viewport.minY())},
      {"viewportMaxX", strings::to_string(viewport.maxX())},
      {"viewportMaxY", strings::to_string(viewport.maxY())},
      {"query", params.m_query},
      {"results", resultString},
  };
  alohalytics::LogEvent("searchEmitResultsAndCoords", stats);
}
}  // namespace

// QueryHandle -------------------------------------------------------------------------------------
QueryHandle::QueryHandle() : m_query(nullptr), m_cancelled(false) {}

void QueryHandle::Cancel()
{
  lock_guard<mutex> lock(m_mu);
  m_cancelled = true;
  if (m_query)
    m_query->Cancel();
}

void QueryHandle::Attach(Query & query)
{
  lock_guard<mutex> lock(m_mu);
  m_query = &query;
  if (m_cancelled)
    m_query->Cancel();
}

void QueryHandle::Detach()
{
  lock_guard<mutex> lock(m_mu);
  m_query = nullptr;
}

// Engine::Params ----------------------------------------------------------------------------------
Engine::Params::Params() : m_locale("en"), m_numThreads(1) {}

Engine::Params::Params(string const & locale, size_t numThreads)
  : m_locale(locale), m_numThreads(numThreads)
{
}

// Engine ------------------------------------------------------------------------------------------
Engine::Engine(Index & index, CategoriesHolder const & categories,
               storage::CountryInfoGetter const & infoGetter,
               unique_ptr<SearchQueryFactory> factory, Params const & params)
  : m_categories(categories), m_shutdown(false)
{
  InitSuggestions doInit;
  m_categories.ForEachName(bind<void>(ref(doInit), _1));
  doInit.GetSuggests(m_suggests);

  m_contexts.resize(params.m_numThreads);
  for (size_t i = 0; i < params.m_numThreads; ++i)
  {
    auto query = factory->BuildSearchQuery(index, m_categories, m_suggests, infoGetter);
    query->SetPreferredLocale(params.m_locale);
    m_contexts[i].m_query = move(query);
  }

  m_threads.reserve(params.m_numThreads);
  for (size_t i = 0; i < params.m_numThreads; ++i)
    m_threads.emplace_back(&Engine::MainLoop, this, ref(m_contexts[i]));
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

weak_ptr<QueryHandle> Engine::Search(SearchParams const & params, m2::RectD const & viewport)
{
  shared_ptr<QueryHandle> handle(new QueryHandle());
  PostMessage(bind(&Engine::DoSearch, this, params, viewport, handle, _1), Message::TYPE_TASK);
  return handle;
}

void Engine::SetSupportOldFormat(bool support)
{
  PostMessage(bind(&Engine::DoSupportOldFormat, this, support, _1), Message::TYPE_BROADCAST);
}

void Engine::ClearCaches()
{
  PostMessage(bind(&Engine::DoClearCaches, this, _1), Message::TYPE_BROADCAST);
}

void Engine::SetRankPivot(SearchParams const & params, m2::RectD const & viewport,
                          bool viewportSearch, Query & query)
{
  if (!viewportSearch && params.IsValidPosition())
  {
    m2::PointD const pos = MercatorBounds::FromLatLon(params.m_lat, params.m_lon);
    if (m2::Inflate(viewport, viewport.SizeX() / 4.0, viewport.SizeY() / 4.0).IsPointInside(pos))
    {
      query.SetRankPivot(pos);
      return;
    }
  }

  query.SetRankPivot(viewport.Center());
}

void Engine::EmitResults(SearchParams const & params, Results const & res)
{
  params.m_onResults(res);
}

void Engine::MainLoop(Context & context)
{
  while (true)
  {
    unique_lock<mutex> lock(m_mu);
    m_cv.wait(lock, [&]()
    {
      return m_shutdown || !m_messages.empty() || !context.m_messages.empty();
    });

    if (m_shutdown)
      break;

    // As SearchEngine is thread-safe, there is a global order on
    // public API requests, and this order is kept by the global
    // |m_messages| queue.  When a broadcast message arrives, it must
    // be executed by all threads, therefore the first free thread
    // extracts it from |m_messages| and replicates to all
    // thread-specific |m_messages| queues.
    bool hasBroadcast = false;
    while (!m_messages.empty() && m_messages.front().m_type == Message::TYPE_BROADCAST)
    {
      for (auto & b : m_contexts)
        b.m_messages.push(m_messages.front());
      m_messages.pop();
      hasBroadcast = true;
    }

    // Consumes first non-broadcast message, if any.
    if (!m_messages.empty())
    {
      context.m_messages.push(move(m_messages.front()));
      m_messages.pop();
    }

    queue<Message> messages;
    messages.swap(context.m_messages);

    lock.unlock();
    if (hasBroadcast)
      m_cv.notify_all();

    while (!messages.empty())
    {
      messages.front()(*context.m_query);
      messages.pop();
    }
  }
}

template <typename... TArgs>
void Engine::PostMessage(TArgs && ... args)
{
  lock_guard<mutex> lock(m_mu);
  m_messages.emplace(forward<TArgs>(args)...);
  m_cv.notify_one();
}

void Engine::DoSearch(SearchParams const & params, m2::RectD const & viewport,
                      shared_ptr<QueryHandle> handle, Query & query)
{
  bool const viewportSearch = params.GetMode() == Mode::Viewport;

  // Initialize query.
  query.Init(viewportSearch);
  handle->Attach(query);
  MY_SCOPE_GUARD(detach, [&handle] { handle->Detach(); });

  // Early exit when query is cancelled.
  if (query.IsCancelled())
  {
    params.m_onResults(Results::GetEndMarker(true /* isCancelled */));
    return;
  }

  SetRankPivot(params, viewport, viewportSearch, query);

  if (params.IsValidPosition())
    query.SetPosition(MercatorBounds::FromLatLon(params.m_lat, params.m_lon));
  else
    query.SetPosition(viewport.Center());

  query.SetMode(params.GetMode());

  // This flag is needed for consistency with old search algorithm
  // only. It will be gone when we remove old search code.
  query.SetSearchInWorld(true);

  query.SetInputLocale(params.m_inputLocale);

  ASSERT(!params.m_query.empty(), ());
  query.SetQuery(params.m_query);

  Results res;

  query.SearchCoordinates(res);

  try
  {
    if (params.m_onStarted)
      params.m_onStarted();

    if (viewportSearch)
    {
      query.SetViewport(viewport, true /* forceUpdate */);
      query.SearchViewportPoints(res);
    }
    else
    {
      query.SetViewport(viewport, params.IsSearchAroundPosition() /* forceUpdate */);
      query.Search(res, kResultsCount);
    }

    EmitResults(params, res);
  }
  catch (Query::CancelException const &)
  {
    LOG(LDEBUG, ("Search has been cancelled."));
  }

  if (!viewportSearch && !query.IsCancelled())
    SendStatistics(params, viewport, res);

  // Emit finish marker to client.
  params.m_onResults(Results::GetEndMarker(query.IsCancelled()));
}

void Engine::DoSupportOldFormat(bool support, Query & query) { query.SupportOldFormat(support); }

void Engine::DoClearCaches(Query & query) { query.ClearCaches(); }
}  // namespace search
