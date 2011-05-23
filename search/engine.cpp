#include "engine.hpp"
#include "query.hpp"
#include "result.hpp"
#include "../indexer/feature.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace search
{

Engine::Engine(IndexType const * pIndex) : m_pIndex(pIndex)
{
}

void Engine::Search(string const & queryText,
                    m2::RectD const & rect,
                    function<void (Result const &)> const & f)
{
  impl::Query query(queryText, rect, m_pIndex);
  query.Search(f);
}

}  // namespace search
