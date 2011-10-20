#include "search_engine.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/search_string_utils.hpp"

#include "../base/logging.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace search
{

Engine::Engine(IndexType const * pIndex, CategoriesHolder * pCategories)
  : m_pIndex(pIndex)
{
  for (CategoriesHolder::const_iterator it = pCategories->begin(); it != pCategories->end(); ++it)
  {
    for (size_t i = 0; i < it->m_synonyms.size(); ++i)
    {
      vector<uint32_t> & types = m_categories[NormalizeAndSimplifyString(it->m_synonyms[i].m_name)];
      types.insert(types.end(), it->m_types.begin(), it->m_types.end());
    }
  }

  delete pCategories;

  m_pQuery.reset(new Query(pIndex, &m_categories));
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
