#include "routing/index_graph_serializer.hpp"

namespace routing
{
// static
uint8_t constexpr IndexGraphSerializer::kVersion;

bool IndexGraphSerializer::IsCarRoad(unordered_set<uint32_t> const & carFeatureIds,
                                     uint32_t featureId)
{
  return carFeatureIds.find(featureId) != carFeatureIds.cend();
}

bool IndexGraphSerializer::IsCarJoint(IndexGraph const & graph,
                                      unordered_set<uint32_t> const & carFeatureIds,
                                      Joint::Id jointId)
{
  uint32_t count = 0;

  graph.ForEachPoint(jointId, [&](RoadPoint const & rp) {
    if (IsCarRoad(carFeatureIds, rp.GetFeatureId()))
      ++count;
  });

  return count >= 2;
}

uint32_t IndexGraphSerializer::CalcNumCarRoads(IndexGraph const & graph,
                                               unordered_set<uint32_t> const & carFeatureIds)
{
  uint32_t result = 0;

  graph.ForEachRoad([&](uint32_t featureId, RoadJointIds const & /* road */) {
    if (IsCarRoad(carFeatureIds, featureId))
      ++result;
  });

  return result;
}

Joint::Id IndexGraphSerializer::CalcNumCarJoints(IndexGraph const & graph,
                                                 unordered_set<uint32_t> const & carFeatureIds)
{
  Joint::Id result = 0;

  for (Joint::Id jointId = 0; jointId < graph.GetNumJoints(); ++jointId)
  {
    if (IsCarJoint(graph, carFeatureIds, jointId))
      ++result;
  }

  return result;
}

void IndexGraphSerializer::MakeFeatureIds(IndexGraph const & graph,
                                          unordered_set<uint32_t> const & carFeatureIds,
                                          vector<uint32_t> & featureIds)
{
  featureIds.clear();
  featureIds.reserve(graph.GetNumRoads());

  graph.ForEachRoad([&](uint32_t featureId, RoadJointIds const & /* road */) {
    featureIds.push_back(featureId);
  });

  sort(featureIds.begin(), featureIds.end(), [&](uint32_t featureId0, uint32_t featureId1) {
    bool const isCar0 = IsCarRoad(carFeatureIds, featureId0);
    bool const isCar1 = IsCarRoad(carFeatureIds, featureId1);
    if (isCar0 != isCar1)
      return isCar0;

    return featureId0 < featureId1;
  });
}
}  // namespace routing
