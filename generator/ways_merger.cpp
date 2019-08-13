#include "generator/ways_merger.hpp"

namespace generator
{
AreaWayMerger::AreaWayMerger( std::shared_ptr<generator::cache::IntermediateDataReader> const & cache) :
  m_cache(cache)
{
}

void AreaWayMerger::AddWay(uint64_t id)
{
  auto e = std::make_shared<WayElement>(id);
  if (m_cache->GetWay(id, *e) && e->IsValid())
  {
    m_map.emplace(e->nodes.front(), e);
    m_map.emplace(e->nodes.back(), e);
  }
}
}  // namespace generator
