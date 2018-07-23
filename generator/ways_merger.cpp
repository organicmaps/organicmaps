#include "generator/ways_merger.hpp"

namespace generator
{
AreaWayMerger::AreaWayMerger(cache::IntermediateDataReader & holder) :
  m_holder(holder)
{
}

void AreaWayMerger::AddWay(uint64_t id)
{
  auto e = std::make_shared<WayElement>(id);
  if (m_holder.GetWay(id, *e) && e->IsValid())
  {
    m_map.emplace(e->nodes.front(), e);
    m_map.emplace(e->nodes.back(), e);
  }
}
}  // namespace generator
