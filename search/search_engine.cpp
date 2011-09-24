#include "search_engine.hpp"
#include "categories_holder.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../base/logging.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace search
{

Engine::Engine(IndexType const * pIndex, CategoriesHolder * pCategories)
  : m_pIndex(pIndex), m_pCategories(pCategories)
{
}

Engine::~Engine()
{
}

void Engine::Search(string const & queryText,
                    m2::RectD const & viewport,
                    function<void (Result const &)> const & f)
{
  LOG(LDEBUG, (queryText, viewport));
  search::Query query(m_pIndex, m_pCategories.get());
  query.Search(queryText, viewport, f);
  f(Result::GetEndResult());
}

}  // namespace search
