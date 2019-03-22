#pragma once

#include "generator/relation_tags.hpp"

struct OsmElement;

namespace generator
{
namespace cache
{
class IntermediateDataReader;
}  // namespace cache

// Class RelationTagsEnricher enriches relation element tags.
class RelationTagsEnricher
{
public:
  explicit RelationTagsEnricher(cache::IntermediateDataReader & cache);
  void operator()(OsmElement & p);

private:
  cache::IntermediateDataReader & m_cache;
  RelationTagsNode m_nodeRelations;
  RelationTagsWay m_wayRelations;
};
}  // namespace generator
