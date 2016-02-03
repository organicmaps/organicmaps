#include "search_engine.hpp"

#include "categories_holder.hpp"
#include "geometry_utils.hpp"
#include "search_query.hpp"
#include "search_string_utils.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"

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
    if (name.m_prefixLengthToSuggest != CategoriesHolder::Category::EMPTY_PREFIX_LENGTH)
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

Engine::Engine(Index & index, Reader * categoriesR, storage::CountryInfoGetter const & infoGetter,
               string const & locale, unique_ptr<SearchQueryFactory> && factory)
  : m_categories(categoriesR), m_factory(move(factory)), m_shutdown(false)
{
  InitSuggestions doInit;
  m_categories.ForEachName(bind<void>(ref(doInit), _1));
  doInit.GetSuggests(m_suggests);

  m_query = m_factory->BuildSearchQuery(index, m_categories, m_suggests, infoGetter);
  m_query->SetPreferredLocale(locale);

  m_thread = threads::SimpleThread(&Engine::MainLoop, this);
}

Engine::~Engine()
{
  {
    lock_guard<mutex> lock(m_mu);
    m_shutdown = true;
    m_cv.notify_one();
  }
  m_thread.join();
}

weak_ptr<QueryHandle> Engine::Search(SearchParams const & params, m2::RectD const & viewport)
{
  shared_ptr<QueryHandle> handle(new QueryHandle());
  PostTask(bind(&Engine::DoSearch, this, params, viewport, handle));
  return handle;
}

void Engine::SetSupportOldFormat(bool support)
{
  PostTask(bind(&Engine::DoSupportOldFormat, this, support));
}

void Engine::ClearCaches() { PostTask(bind(&Engine::DoClearCaches, this)); }

bool Engine::GetNameByType(uint32_t type, int8_t locale, string & name) const
{
  uint8_t level = ftype::GetLevel(type);
  ASSERT_GREATER(level, 0, ());

  while (true)
  {
    if (m_categories.GetNameByType(type, locale, name))
      return true;

    if (--level == 0)
      break;

    ftype::TruncValue(type, level);
  }

  return false;
}

void Engine::SetRankPivot(SearchParams const & params,
                          m2::RectD const & viewport, bool viewportSearch)
{
  if (!viewportSearch && params.IsValidPosition())
  {
    m2::PointD const pos = MercatorBounds::FromLatLon(params.m_lat, params.m_lon);
    if (m2::Inflate(viewport, viewport.SizeX() / 4.0, viewport.SizeY() / 4.0).IsPointInside(pos))
    {
      m_query->SetRankPivot(pos);
      return;
    }
  }

  m_query->SetRankPivot(viewport.Center());
}

void Engine::EmitResults(SearchParams const & params, Results const & res)
{
  params.m_callback(res);
}

void Engine::MainLoop()
{
  while (true)
  {
    unique_lock<mutex> lock(m_mu);
    m_cv.wait(lock, [this]()
    {
      return m_shutdown || !m_tasks.empty();
    });

    if (m_shutdown)
      break;

    function<void()> task(move(m_tasks.front()));
    m_tasks.pop();
    lock.unlock();

    task();
  }
}

void Engine::PostTask(function<void()> && task)
{
  lock_guard<mutex> lock(m_mu);
  m_tasks.push(move(task));
  m_cv.notify_one();
}

void Engine::DoSearch(SearchParams const & params, m2::RectD const & viewport,
                      shared_ptr<QueryHandle> handle)
{
  bool const viewportSearch = params.HasSearchMode(SearchParams::IN_VIEWPORT_ONLY);

  // Initialize query.
  m_query->Init(viewportSearch);
  handle->Attach(*m_query);
  MY_SCOPE_GUARD(detach, [&handle] { handle->Detach(); });

  // Early exit when query is cancelled.
  if (m_query->IsCancelled())
  {
    params.m_callback(Results::GetEndMarker(true /* isCancelled */));
    return;
  }

  SetRankPivot(params, viewport, viewportSearch);

  if (params.IsValidPosition())
    m_query->SetPosition(MercatorBounds::FromLatLon(params.m_lat, params.m_lon));
  else
    m_query->SetPosition(viewport.Center());

  m_query->SetSearchInWorld(params.HasSearchMode(SearchParams::SEARCH_WORLD));
  m_query->SetInputLocale(params.m_inputLocale);

  ASSERT(!params.m_query.empty(), ());
  m_query->SetQuery(params.m_query);

  Results res;

  m_query->SearchCoordinates(res);

  try
  {
    if (viewportSearch)
    {
      m_query->SetViewport(viewport, true /* forceUpdate */);
      m_query->SearchViewportPoints(res);
    }
    else
    {
      m_query->SetViewport(viewport, params.IsSearchAroundPosition() /* forceUpdate */);
      m_query->Search(res, kResultsCount);
    }

    EmitResults(params, res);
  }
  catch (Query::CancelException const &)
  {
    LOG(LDEBUG, ("Search has been cancelled."));
  }

  if (!viewportSearch && !m_query->IsCancelled())
    SendStatistics(params, viewport, res);

  // Emit finish marker to client.
  params.m_callback(Results::GetEndMarker(m_query->IsCancelled()));
}

void Engine::DoSupportOldFormat(bool support) { m_query->SupportOldFormat(support); }

void Engine::DoClearCaches() { m_query->ClearCaches(); }
}  // namespace search
