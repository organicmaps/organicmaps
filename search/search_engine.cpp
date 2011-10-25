#include "search_engine.hpp"
#include "category_info.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/search_string_utils.hpp"

#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace search
{

Engine::Engine(IndexType const * pIndex, CategoriesHolder * pCategories,
               ModelReaderPtr polyR, ModelReaderPtr countryR)
: m_pIndex(pIndex), m_pCategories(new map<strings::UniString, CategoryInfo>()),
  m_infoGetter(polyR, countryR)
{
  for (CategoriesHolder::const_iterator it = pCategories->begin(); it != pCategories->end(); ++it)
  {
    for (size_t i = 0; i < it->m_synonyms.size(); ++i)
    {
      CategoryInfo & info = (*m_pCategories)[NormalizeAndSimplifyString(it->m_synonyms[i].m_name)];
      info.m_types.insert(info.m_types.end(), it->m_types.begin(), it->m_types.end());
      info.m_prefixLengthToSuggest = min(info.m_prefixLengthToSuggest,
                                         it->m_synonyms[i].m_prefixLengthToSuggest);
    }
  }
  delete pCategories;

  m_pQuery.reset(new Query(pIndex, m_pCategories.get(), &m_infoGetter));
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
