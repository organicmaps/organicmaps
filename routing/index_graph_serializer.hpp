#pragma once

#include "routing/index_graph.hpp"
#include "routing/routing_exception.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/bits.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"

namespace routing
{
class IndexGraphSerializer final
{
public:
  IndexGraphSerializer() = delete;

  template <class Sink>
  static void Serialize(IndexGraph const & graph, Sink & sink)
  {
    WriteToSink(sink, kVersion);

    uint32_t const numJoints = graph.GetNumJoints();
    WriteToSink(sink, numJoints);
    uint32_t const bitsPerJoint = bits::NumUsedBits(numJoints);

    map<uint32_t, RoadJointIds> roads;
    graph.ForEachRoad(
        [&](uint32_t featureId, RoadJointIds const & road) { roads[featureId] = road; });

    WriteToSink(sink, static_cast<uint32_t>(roads.size()));

    BitWriter<Sink> writer(sink);

    // -1 for uint32_t is some confusing, but it allows process first iteration in the common way.
    // It works because uint32_t is modular ring type.
    uint32_t prevFeatureId = -1;

    for (auto const & it : roads)
    {
      uint32_t const featureId = it.first;
      RoadJointIds const & road = it.second;

      uint32_t const featureDelta = featureId - prevFeatureId;
      if (!Encode32(writer, featureDelta))
      {
        MYTHROW(RoutingException, ("Can't encode featureDelta =", featureDelta, ", prevFeatureId =",
                                   prevFeatureId, ", featureId =", featureId));
      }

      prevFeatureId = featureId;

      uint32_t const pointCap = road.GetMaxPointId() + 1;
      if (!Encode32(writer, pointCap))
      {
        MYTHROW(RoutingException,
                ("Can't encode pointCap =", pointCap, ", featureId =", featureId));
      }

      uint32_t prevPointId = -1;
      road.ForEachJoint([&](uint32_t pointId, Joint::Id jointId) {
        uint32_t const pointDelta = pointId - prevPointId;
        if (!Encode32(writer, pointDelta))
        {
          MYTHROW(RoutingException,
                  ("Can't encode pointDelta =", pointDelta, ", prevPointId =", prevPointId,
                   ", pointId =", pointId, ", featureId =", featureId));
        }

        prevPointId = pointId;
        writer.WriteAtMost32Bits(jointId, bitsPerJoint);
      });
    }
  }

  template <class Source>
  static void Deserialize(IndexGraph & graph, Source & src)
  {
    uint8_t const version = ReadPrimitiveFromSource<uint8_t>(src);
    if (version != kVersion)
    {
      MYTHROW(RoutingException,
              ("Unknown index graph version =", version, ", current version =", kVersion));
    }

    uint32_t const numJoints = ReadPrimitiveFromSource<uint32_t>(src);
    uint32_t const bitsPerJoint = bits::NumUsedBits(numJoints);
    uint32_t const numRoads = ReadPrimitiveFromSource<uint32_t>(src);

    BitReader<Source> reader(src);

    uint32_t featureId = -1;
    for (uint32_t i = 0; i < numRoads; ++i)
    {
      uint32_t const featureDelta = Decode32(reader);
      featureId += featureDelta;

      uint32_t const pointCap = Decode32(reader);
      if (pointCap < 1)
        MYTHROW(RoutingException, ("Invalid pointCap =", pointCap, ", featureId =", featureId));

      uint32_t const maxPointId = pointCap - 1;

      RoadJointIds & roadJoints = graph.InitRoad(featureId, maxPointId);

      for (uint32_t pointId = -1; pointId != maxPointId;)
      {
        uint32_t const pointDelta = Decode32(reader);
        pointId += pointDelta;
        if (pointId > maxPointId)
        {
          MYTHROW(RoutingException, ("Invalid pointId =", pointId, ", maxPointId =", maxPointId,
                                     ", pointDelta =", pointDelta, ", featureId =", featureId));
        }

        Joint::Id const jointId = reader.ReadAtMost32Bits(bitsPerJoint);
        if (jointId >= numJoints)
        {
          MYTHROW(RoutingException, ("Invalid jointId =", jointId, ", numJoints =", numJoints,
                                     ", pointId =", pointId, ", featureId =", featureId));
        }

        roadJoints.AddJoint(pointId, jointId);
      }
    }

    graph.Build(numJoints);
  }

private:
  static uint8_t constexpr kVersion = 0;

  template <typename TWriter>
  static bool Encode32(BitWriter<TWriter> & writer, uint32_t value)
  {
    return coding::GammaCoder::Encode(writer, static_cast<uint64_t>(value));
  }

  template <class Source>
  static uint32_t Decode32(BitReader<Source> & reader)
  {
    uint64_t const decoded = coding::GammaCoder::Decode(reader);
    if (decoded > numeric_limits<uint32_t>::max())
      MYTHROW(RoutingException, ("Invalid uint32_t decoded", decoded));

    return static_cast<uint32_t>(decoded);
  }
};
}  // namespace routing
