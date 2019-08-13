#include "generator/relation_tags_enricher.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

namespace generator
{
RelationTagsEnricher::RelationTagsEnricher(std::shared_ptr<cache::IntermediateDataReader> const & cache)
  : m_cache(cache) {}

void RelationTagsEnricher::operator()(OsmElement & p)
{
  if (p.IsNode())
  {
    m_nodeRelations.Reset(p.m_id, &p);
    m_cache->ForEachRelationByNodeCached(p.m_id, m_nodeRelations);
  }
  else if (p.IsWay())
  {
    m_wayRelations.Reset(p.m_id, &p);
    m_cache->ForEachRelationByWayCached(p.m_id, m_wayRelations);
  }
}
}  // namespace generator
