#include "search/localities_source.hpp"

#include "indexer/classificator.hpp"

namespace search
{
LocalitiesSource::LocalitiesSource()
{
  auto & c = classif();
  m_city = c.GetTypeByPath({"place", "city"});
  m_town = c.GetTypeByPath({"place", "town"});
}
}  // namespace search
