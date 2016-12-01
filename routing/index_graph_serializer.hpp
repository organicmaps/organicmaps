#pragma once

#include "routing/index_graph.hpp"
#include "routing/routing_exception.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"
#include "std/unordered_map.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

namespace routing
{
class IndexGraphSerializer final
{
public:
  IndexGraphSerializer() = delete;

  template <class Sink>
  static void Serialize(IndexGraph const & graph, unordered_set<uint32_t> const & carFeatureIds,
                        Sink & sink)
  {
    Header const header(graph, carFeatureIds);
    header.Serialize(sink);

    vector<uint32_t> featureIds;
    MakeFeatureIds(graph, carFeatureIds, featureIds);

    BitWriter<Sink> writer(sink);
    JointIdEncoder carEncoder(header, true);
    JointIdEncoder restEncoder(header, false);

    // -1 for uint32_t is some confusing, but it allows process first iteration in the common way.
    // It works because uint32_t is modular ring type.
    uint32_t prevFeatureId = -1;

    for (uint32_t const featureId : featureIds)
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

        if (IsCarJoint(graph, carFeatureIds, jointId))
          carEncoder.Write(jointId, writer);
        else
          restEncoder.Write(jointId, writer);
      });
    }
  }

  template <class Source>
  static void Deserialize(IndexGraph & graph, Source & src, bool forCar)
  {
    Header header;
    header.Deserialize(src);

    BitReader<Source> reader(src);
    JointIdDecoder jointIdDecoder(header);

    uint32_t featureId = -1;
    uint32_t const roadsLimit = forCar ? header.GetNumCarRoads() : header.GetNumRoads();
    for (uint32_t i = 0; i < roadsLimit; ++i)
    {
      uint32_t const featureDelta = Read32(reader);
      featureId += featureDelta;

      uint32_t const pointCap = Read32(reader);
      if (pointCap < 1)
        MYTHROW(RoutingException, ("Invalid pointCap =", pointCap, ", featureId =", featureId));

      uint32_t const maxPointId = pointCap - 1;

      RoadJointIds & roadJoints = graph.InitRoad(featureId, maxPointId);

      for (uint32_t pointId = -1; pointId != maxPointId;)
      {
        uint32_t const pointDelta = Read32(reader);
        pointId += pointDelta;
        if (pointId > maxPointId)
        {
          MYTHROW(RoutingException, ("Invalid pointId =", pointId, ", maxPointId =", maxPointId,
                                     ", pointDelta =", pointDelta, ", featureId =", featureId));
        }

        Joint::Id const jointId = jointIdDecoder.Read(reader);
        if (jointId >= header.GetNumJoints())
        {
          MYTHROW(RoutingException,
                  ("Invalid jointId =", jointId, ", numJoints =", header.GetNumJoints(),
                   ", pointId =", pointId, ", featureId =", featureId));
        }

        if (!forCar || jointId < header.GetNumCarJoints())
          roadJoints.AddJoint(pointId, jointId);
      }
    }

    if (jointIdDecoder.GetCarCount() != header.GetNumCarJoints())
    {
      MYTHROW(RoutingException,
              ("Invalid JointIdDecoder.GetCarCount() =", jointIdDecoder.GetCarCount(),
               ", expected =", header.GetNumCarJoints()));
    }

    if (!forCar && jointIdDecoder.GetRestCount() != header.GetNumJoints())
    {
      MYTHROW(RoutingException,
              ("Invalid JointIdDecoder.GetRestCount() =", jointIdDecoder.GetRestCount(),
               ", expected =", header.GetNumJoints()));
    }

    graph.Build(forCar ? header.GetNumCarJoints() : header.GetNumJoints());
  }

private:
  static uint8_t constexpr kVersion = 0;

  template <typename Sink>
  static void Write32(BitWriter<Sink> & writer, uint32_t value)
  {
    bool const success = coding::GammaCoder::Encode(writer, static_cast<uint64_t>(value));
    if (!success)
      MYTHROW(RoutingException, ("Can't encode", value));
  }

  template <class Source>
  static uint32_t Read32(BitReader<Source> & reader)
  {
    uint64_t const decoded = coding::GammaCoder::Decode(reader);
    if (decoded > numeric_limits<uint32_t>::max())
      MYTHROW(RoutingException, ("Decoded uint32_t out of limit", decoded));

    return static_cast<uint32_t>(decoded);
  }

  static bool IsCarRoad(unordered_set<uint32_t> const & carFeatureIds, uint32_t featureId);
  static bool IsCarJoint(IndexGraph const & graph, unordered_set<uint32_t> const & carFeatureIds,
                         Joint::Id jointId);
  static uint32_t CalcNumCarRoads(IndexGraph const & graph,
                                  unordered_set<uint32_t> const & carFeatureIds);
  static Joint::Id CalcNumCarJoints(IndexGraph const & graph,
                                    unordered_set<uint32_t> const & carFeatureIds);
  static void MakeFeatureIds(IndexGraph const & graph,
                             unordered_set<uint32_t> const & carFeatureIds,
                             vector<uint32_t> & featureIds);

  class Header final
  {
  public:
    Header() = default;

    Header(IndexGraph const & graph, unordered_set<uint32_t> const & carFeatureIds)
      : m_numRoads(graph.GetNumRoads())
      , m_numJoints(graph.GetNumJoints())
      , m_numCarRoads(CalcNumCarRoads(graph, carFeatureIds))
      , m_numCarJoints(CalcNumCarJoints(graph, carFeatureIds))
    {
    }

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_numRoads);
      WriteToSink(sink, m_numJoints);
      WriteToSink(sink, m_numCarRoads);
      WriteToSink(sink, m_numCarJoints);
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_version = ReadPrimitiveFromSource<decltype(m_version)>(src);
      if (m_version != kVersion)
      {
        MYTHROW(RoutingException,
                ("Unknown index graph version ", m_version, ", current version ", kVersion));
      }

      m_numRoads = ReadPrimitiveFromSource<decltype(m_numRoads)>(src);
      m_numJoints = ReadPrimitiveFromSource<decltype(m_numJoints)>(src);
      m_numCarRoads = ReadPrimitiveFromSource<decltype(m_numCarRoads)>(src);
      m_numCarJoints = ReadPrimitiveFromSource<decltype(m_numCarJoints)>(src);
    }

    uint32_t GetNumRoads() const { return m_numRoads; }
    Joint::Id GetNumJoints() const { return m_numJoints; }
    uint32_t GetNumCarRoads() const { return m_numCarRoads; }
    Joint::Id GetNumCarJoints() const { return m_numCarJoints; }

  private:
    uint8_t m_version = kVersion;
    uint32_t m_numRoads = 0;
    Joint::Id m_numJoints = 0;
    uint32_t m_numCarRoads = 0;
    Joint::Id m_numCarJoints = 0;
  };

  class JointIdEncoder final
  {
  public:
    JointIdEncoder(Header const & header, bool forCar)
      : m_forCar(forCar), m_count(forCar ? 0 : header.GetNumCarJoints())
    {
    }

    template <class Sink>
    void Write(Joint::Id originalId, BitWriter<Sink> & writer)
    {
      writer.Write(m_forCar ? 0 : 1, 1);

      auto const & it = m_convertedIds.find(originalId);
      if (it != m_convertedIds.end())
      {
        Joint::Id const convertedId = it->second;
        if (convertedId >= m_count)
        {
          MYTHROW(RoutingException,
                  ("Dublicated joint id should be less then count, convertedId =", convertedId,
                   ", count =", m_count, ", originalId =", originalId));
        }
        writer.Write(1, 1);
        Write32(writer, m_count - convertedId);
        return;
      }

      m_convertedIds[originalId] = m_count;
      writer.Write(0, 1);
      ++m_count;
    }

    Joint::Id GetCount() const { return m_count; }

  private:
    bool const m_forCar;
    Joint::Id m_count;
    unordered_map<Joint::Id, Joint::Id> m_convertedIds;
  };

  class JointIdDecoder final
  {
  public:
    JointIdDecoder(Header const & header) : m_CarCount(0), m_RestCount(header.GetNumCarJoints()) {}

    template <class Source>
    Joint::Id Read(BitReader<Source> & reader)
    {
      uint8_t const carBit = reader.Read(1);
      Joint::Id & count = carBit == 0 ? m_CarCount : m_RestCount;

      uint8_t const fullId = reader.Read(1);
      if (fullId)
        return count - Read32(reader);

      return count++;
    }

    Joint::Id GetCarCount() const { return m_CarCount; }
    Joint::Id GetRestCount() const { return m_RestCount; }

  private:
    Joint::Id m_CarCount;
    Joint::Id m_RestCount;
  };
};
}  // namespace routing
