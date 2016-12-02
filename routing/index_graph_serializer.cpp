#include "routing/index_graph_serializer.hpp"

namespace routing
{
// static
uint8_t constexpr IndexGraphSerializer::kVersion;

void IndexGraphSerializer::SectionSerializer::SerializeToBuffer(
    IndexGraph const & graph, unordered_map<uint32_t, VehicleMask> const & masks,
    JointIdEncoder & jointEncoder)
{
  m_buffer.clear();
  MemWriter<vector<uint8_t>> memWriter(m_buffer);
  BitWriter<MemWriter<vector<uint8_t>>> writer(memWriter);

  // -1 for uint32_t is some confusing, but it allows process first iteration in the common way.
  // It works because uint32_t is modular ring type.
  uint32_t prevFeatureId = -1;

  for (uint32_t const featureId : m_featureIds)
  {
    RoadJointIds const & road = graph.GetRoad(featureId);

    uint32_t const featureDelta = featureId - prevFeatureId;
    Write32(writer, featureDelta);
    prevFeatureId = featureId;

    uint32_t const pointCap = road.GetMaxPointId() + 1;
    Write32(writer, pointCap);

    uint32_t prevPointId = -1;
    road.ForEachJoint([&](uint32_t pointId, Joint::Id jointId) {
      uint32_t const pointDelta = pointId - prevPointId;
      Write32(writer, pointDelta);
      prevPointId = pointId;

      jointEncoder.Write(jointId, writer);
    });
  }
}

// static
VehicleMask IndexGraphSerializer::GetRoadMask(unordered_map<uint32_t, VehicleMask> const & masks,
                                              uint32_t featureId)
{
  auto const & it = masks.find(featureId);
  if (it == masks.cend())
    MYTHROW(RoutingException, ("Can't find vehicle mask for feature", featureId));

  return it->second;
}

// static
VehicleMask IndexGraphSerializer::GetJointMask(IndexGraph const & graph,
                                               unordered_map<uint32_t, VehicleMask> const & masks,
                                               Joint::Id jointId)
{
  // Joint mask need two counts from road masks.
  // For example, Car joint should contains at least two car roads.
  VehicleMask countOne = 0;
  VehicleMask countTwo = 0;

  graph.ForEachPoint(jointId, [&](RoadPoint const & rp) {
    VehicleMask const roadMask = GetRoadMask(masks, rp.GetFeatureId());
    countOne |= roadMask;
    countTwo |= roadMask & countOne;
  });

  return countTwo;
}

// static
void IndexGraphSerializer::PrepareSectionSerializers(
    IndexGraph const & graph, unordered_map<uint32_t, VehicleMask> const & masks,
    vector<SectionSerializer> & serializers)
{
  // It would be a bit faster to read sections in reverse order for the car couter.
  // kCarMask = 4, therefore first 4 sections are cars sections in reverse order.
  for (VehicleMask mask = kNumVehicleMasks - 1; mask > 0; --mask)
    serializers.emplace_back(mask);

  graph.ForEachRoad([&](uint32_t featureId, RoadJointIds const & /* road */) {
    VehicleMask const mask = GetRoadMask(masks, featureId);
    uint32_t const index = kNumVehicleMasks - 1 - mask;
    if (index >= serializers.size())
      MYTHROW(RoutingException, ("Mask index", index, "out of bounds", serializers.size(),
                                 ", road mask =", mask, ", featureId =", featureId));

    SectionSerializer & serializer = serializers[index];
    if (serializer.GetMask() != mask)
      MYTHROW(RoutingException,
              ("Wrong serializer, index =", index, ", serializer mask =", serializer.GetMask(),
               ", road mask =", mask, ", featureId =", featureId));

    serializer.AddRoad(featureId);
  });

  for (SectionSerializer & serializer : serializers)
    serializer.SortRoads();
}
}  // namespace routing
