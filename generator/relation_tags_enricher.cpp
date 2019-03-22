#include "generator/relation_tags_enricher.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

namespace generator
{
RelationTagsEnricher::RelationTagsEnricher(cache::IntermediateDataReader & cache)
  : m_cache(cache) {}

void RelationTagsEnricher::operator()(OsmElement & p)
{
  if (p.IsNode())
  {
    m_nodeRelations.Reset(p.id, &p);
    m_cache.ForEachRelationByNodeCached(p.id, m_nodeRelations);
  }
  else if (p.IsWay())
  {
    m_wayRelations.Reset(p.id, &p);
    m_cache.ForEachRelationByWayCached(p.id, m_wayRelations);
  }
}
}  // namespace generator
