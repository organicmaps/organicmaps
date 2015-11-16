#include "search/search_query_base.hpp"

namespace search
{
SearchQueryBase::SearchQueryBase(Index & index, CategoriesHolder const & categories,
                                 vector<Suggest> const & suggests,
                                 storage::CountryInfoGetter const & infoGetter)
  : m_index(index), m_categories(categories), m_suggests(suggests), m_infoGetter(infoGetter)
{
}
}  // namespace search
