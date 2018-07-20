#include "generator/ways_merger.hpp"

namespace generator {

AreaWayMerger::AreaWayMerger(cache::IntermediateDataReader & holder) :
  m_holder(holder)
{
}

void AreaWayMerger::AddWay(uint64_t id)
{
  std::shared_ptr<WayElement> e(new WayElement(id));
  if (m_holder.GetWay(id, *e) && e->IsValid())
  {
    m_map.insert(make_pair(e->nodes.front(), e));
    m_map.insert(make_pair(e->nodes.back(), e));
  }
}

}  // namespace generator
