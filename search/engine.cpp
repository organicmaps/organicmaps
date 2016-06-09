#include "search/engine.hpp"

#include "geometry_utils.hpp"
#include "processor.hpp"

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
      {"locale", params.m_inputLocale},
      {"results", resultString},
  };
  alohalytics::LogEvent("searchEmitResultsAndCoords", stats);
}
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

Engine::Params::Params(string const & locale, size_t numThreads)
  : m_locale(locale), m_numThreads(numThreads)
{
}

// Engine ------------------------------------------------------------------------------------------
Engine::Engine(Index & index, CategoriesHolder const & categories,
               storage::CountryInfoGetter const & infoGetter, unique_ptr<ProcessorFactory> factory,
               Params const & params)
  : m_categories(categories), m_shutdown(false)
{
  InitSuggestions doInit;
  m_categories.ForEachName(bind<void>(ref(doInit), _1));
  doInit.GetSuggests(m_suggests);

  m_contexts.resize(params.m_numThreads);
  for (size_t i = 0; i < params.m_numThreads; ++i)
  {
    auto processor = factory->Build(index, m_categories, m_suggests, infoGetter);
    processor->SetPreferredLocale(params.m_locale);
    m_contexts[i].m_processor = move(processor);
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

weak_ptr<ProcessorHandle> Engine::Search(SearchParams const & params, m2::RectD const & viewport)
{
  shared_ptr<ProcessorHandle> handle(new ProcessorHandle());
  PostMessage(Message::TYPE_TASK, [this, params, viewport, handle](Processor & processor)
              {
                DoSearch(params, viewport, handle, processor);
              });
  return handle;
}

void Engine::SetSupportOldFormat(bool support)
{
  PostMessage(Message::TYPE_BROADCAST, [this, support](Processor & processor)
              {
                processor.SupportOldFormat(support);
              });
}

void Engine::SetLocale(string const & locale)
{
  PostMessage(Message::TYPE_BROADCAST, [this, locale](Processor & processor)
              {
                processor.SetPreferredLocale(locale);
              });
}

void Engine::ClearCaches()
{
  PostMessage(Message::TYPE_BROADCAST, [this](Processor & processor)
              {
                processor.ClearCaches();
              });
}

void Engine::SetRankPivot(SearchParams const & params, m2::RectD const & viewport,
                          bool viewportSearch, Processor & processor)
{
  if (!viewportSearch && params.IsValidPosition())
  {
    m2::PointD const pos = MercatorBounds::FromLatLon(params.m_lat, params.m_lon);
    if (m2::Inflate(viewport, viewport.SizeX() / 4.0, viewport.SizeY() / 4.0).IsPointInside(pos))
    {
      processor.SetRankPivot(pos);
      return;
    }
  }

  processor.SetRankPivot(viewport.Center());
}

void Engine::EmitResults(SearchParams const & params, Results const & res)
{
  params.m_onResults(res);
}

void Engine::MainLoop(Context & context)
{
  while (true)
  {
    bool hasBroadcast = false;
    queue<Message> messages;

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
        context.m_messages.push(move(m_messages.front()));
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

template <typename... TArgs>
void Engine::PostMessage(TArgs &&... args)
{
  lock_guard<mutex> lock(m_mu);
  m_messages.emplace(forward<TArgs>(args)...);
  m_cv.notify_one();
}

void Engine::DoSearch(SearchParams const & params, m2::RectD const & viewport,
                      shared_ptr<ProcessorHandle> handle, Processor & processor)
{
  bool const viewportSearch = params.GetMode() == Mode::Viewport;

  // Initialize query processor.
  processor.Reset();
  processor.Init(viewportSearch);
  handle->Attach(processor);
  MY_SCOPE_GUARD(detach, [&handle]
                 {
                   handle->Detach();
                 });

  // Early exit when query processing is cancelled.
  if (processor.IsCancelled())
  {
    params.m_onResults(Results::GetEndMarker(true /* isCancelled */));
    return;
  }

  SetRankPivot(params, viewport, viewportSearch, processor);

  if (params.IsValidPosition())
    processor.SetPosition(MercatorBounds::FromLatLon(params.m_lat, params.m_lon));
  else
    processor.SetPosition(viewport.Center());

  processor.SetMode(params.GetMode());
  processor.SetSuggestsEnabled(params.GetSuggestsEnabled());

  // This flag is needed for consistency with old search algorithm
  // only. It will be gone when we remove old search code.
  processor.SetSearchInWorld(true);

  processor.SetInputLocale(params.m_inputLocale);

  ASSERT(!params.m_query.empty(), ());
  processor.SetQuery(params.m_query);

  Results res;

  processor.SearchCoordinates(res);

  try
  {
    if (params.m_onStarted)
      params.m_onStarted();

    processor.SetViewport(viewport, true /* forceUpdate */);
    if (viewportSearch)
      processor.SearchViewportPoints(res);
    else
      processor.Search(res, kResultsCount);

    if (!processor.IsCancelled())
      EmitResults(params, res);
  }
  catch (Processor::CancelException const &)
  {
    LOG(LDEBUG, ("Search has been cancelled."));
  }

  if (!viewportSearch && !processor.IsCancelled())
    SendStatistics(params, viewport, res);

  // Emit finish marker to client.
  params.m_onResults(Results::GetEndMarker(processor.IsCancelled()));
}
}  // namespace search
