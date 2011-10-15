#include "search_engine.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../indexer/categories_holder.hpp"

#include "../base/logging.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace search
{

Engine::Engine(IndexType const * pIndex, CategoriesHolder * pCategories)
  : m_pIndex(pIndex), m_pCategories(pCategories)
{
  m_pQuery.reset(new Query(pIndex, pCategories));
}

Engine::~Engine()
{
}

void Engine::SetViewport(m2::RectD const & viewport)
{
  m_pQuery->SetViewport(viewport);
}

void Engine::Search(string const & queryText, function<void (Result const &)> const & f)
{
  LOG(LDEBUG, (queryText));
  m_pQuery->Search(queryText, f);
  f(Result::GetEndResult());
}

}  // namespace search
