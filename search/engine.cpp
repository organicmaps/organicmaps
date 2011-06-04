#include "engine.hpp"
#include "query.hpp"
#include "result.hpp"
#include "../platform/concurrent_runner.hpp"
#include "../indexer/feature.hpp"
#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace search
{

Engine::Engine(IndexType const * pIndex)
  : m_pIndex(pIndex), m_pRunner(new threads::ConcurrentRunner)
{
}

void Engine::Search(string const & queryText,
                    m2::RectD const & rect,
                    function<void (Result const &)> const & f)
{
  {
    threads::MutexGuard mutexGuard(m_mutex);
    UNUSED_VALUE(mutexGuard);
    if (m_pLastQuery)
      m_pLastQuery->SetTerminateFlag();
  }

  impl::Query * pQuery = new impl::Query(queryText, rect, m_pIndex);
  m_pRunner->Run(bind(&impl::Query::SearchAndDestroy, pQuery, f));
}

void Engine::OnQueryDelete()
{
  threads::MutexGuard mutexGuard(m_mutex);
  UNUSED_VALUE(mutexGuard);
  m_pLastQuery = NULL;
}

}  // namespace search
