#include "categories_holder.hpp"
#include "engine.hpp"
#include "query.hpp"
#include "result.hpp"

#include "../platform/concurrent_runner.hpp"

#include "../base/logging.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace search
{

Engine::Engine(IndexType const * pIndex,
               CategoriesHolder & categories)
  : m_pIndex(pIndex), m_pSearchInfo(pIndex->GetWorldSearchInfo()),
    m_pCategories(new CategoriesHolder()),
    m_pRunner(new threads::ConcurrentRunner), m_pLastQuery(NULL),
    m_queriesActive(0)
{
  m_pCategories->swap(categories);
}

Engine::~Engine()
{
  LOG(LDEBUG, (m_queriesActive));
  ASSERT_EQUAL(m_queriesActive, 0, ());
}

void Engine::Search(string const & queryText,
                    m2::RectD const & rect,
                    function<void (Result const &)> const & f)
{
  LOG(LDEBUG, (queryText, rect));

  impl::Query * pQuery =
      new impl::Query(queryText, rect, m_pIndex, this, m_pCategories.get(),
                      m_pSearchInfo->GetTrie(), m_pSearchInfo->GetFeatures());

  {
    threads::MutexGuard mutexGuard(m_mutex);
    UNUSED_VALUE(mutexGuard);

    ASSERT_GREATER_OR_EQUAL(m_queriesActive, 0, ());

    if (m_pLastQuery)
    {
      LOG(LDEBUG, ("Stopping previous", m_pLastQuery->GetQueryText(), m_pLastQuery->GetViewport()));
      m_pLastQuery->SetTerminateFlag();
    }

    m_pLastQuery = pQuery;
    ++m_queriesActive;
    LOG(LDEBUG, ("Queries active", m_queriesActive));
  }

  m_pRunner->Run(bind(&impl::Query::SearchAndDestroy, pQuery, f));
}

void Engine::OnQueryDelete(impl::Query * pQuery)
{
  threads::MutexGuard mutexGuard(m_mutex);
  UNUSED_VALUE(mutexGuard);

  ASSERT_GREATER_OR_EQUAL(m_queriesActive, 1, ());

  --m_queriesActive;
  LOG(LDEBUG, ("Queries active", m_queriesActive));
  LOG(LDEBUG, ("Query destroyed", pQuery->GetQueryText(), pQuery->GetViewport()));

  if (m_pLastQuery == pQuery)
  {
    LOG(LDEBUG, ("Last query destroyed"));
    m_pLastQuery = NULL;
  }
}

void Engine::StopEverything()
{
  threads::MutexGuard mutexGuard(m_mutex);
  UNUSED_VALUE(mutexGuard);

  ASSERT_GREATER_OR_EQUAL(m_queriesActive, 0, ());
  LOG(LINFO, (m_queriesActive, m_pLastQuery));

  if (m_pLastQuery)
  {
    LOG(LDEBUG, ("Stopping previous", m_pLastQuery->GetQueryText(), m_pLastQuery->GetViewport()));
    m_pLastQuery->SetTerminateFlag();
  }
}

}  // namespace search
