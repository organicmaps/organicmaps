#pragma once

#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/routing_exception.hpp"
#include "routing/vehicle_mask.hpp"

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
  static void Serialize(IndexGraph const & graph,
                        unordered_map<uint32_t, VehicleMask> const & masks, Sink & sink)
  {
    Header header(graph, masks);
    JointIdEncoder jointEncoder;

    vector<SectionSerializer> serializers;
    PrepareSectionSerializers(graph, masks, serializers);

    for (uint32_t i = 0; i < serializers.size(); ++i)
    {
      SectionSerializer & serializer = serializers[i];
      Joint::Id const begin = jointEncoder.GetCount();
      serializer.SerializeToBuffer(graph, masks, jointEncoder);
      header.AddSection({serializer.GetMask(), serializer.GetBufferSize(), serializer.GetNumRoads(),
                         begin, jointEncoder.GetCount()});
    }

    header.Serialize(sink);
    for (SectionSerializer & section : serializers)
      section.Flush(sink);
  }

  template <class Source>
  static void Deserialize(IndexGraph & graph, Source & src, VehicleMask requiredMask)
  {
    Header header;
    header.Deserialize(src);

    JointIdConverter jointIdConverter(header.GetNumJoints());

    for (uint32_t i = 0; i < header.GetNumSections(); ++i)
    {
      Section const & section = header.GetSection(i);
      VehicleMask const mask = section.GetMask();

      if (!(mask & requiredMask))
      {
        src.Skip(section.GetSize());
        continue;
      }

      JointIdDecoder jointIdDecoder(section.GetBeginJointId());
      BitReader<Source> reader(src);
      uint64_t const expectedEndPos = src.Pos() + section.GetSize();

      // -1 for uint32_t is some confusing, but it allows process first iteration in the common way.
      // It works because uint32_t is modular ring type.
      uint32_t featureId = -1;
      for (uint32_t i = 0; i < section.GetNumRoads(); ++i)
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

          Joint::Id const jointIdInFile = jointIdDecoder.Read(reader);
          if (jointIdInFile >= section.GetEndJointId())
          {
            MYTHROW(RoutingException,
                    ("Invalid jointId =", jointIdInFile, ", end =", section.GetEndJointId(),
                     ", mask =", mask, ", pointId =", pointId, ", featureId =", featureId));
          }

          Joint::Id const jointId = jointIdConverter.Convert(jointIdInFile);
          // TODO: filter redundant joints
          roadJoints.AddJoint(pointId, jointId);
        }
      }

      if (jointIdDecoder.GetCount() != section.GetEndJointId())
      {
        MYTHROW(RoutingException, ("Invalid decoder count =", jointIdDecoder.GetCount(),
                                   ", expected =", section.GetEndJointId(), ", mask =", mask));
      }

      if (src.Pos() != expectedEndPos)
      {
        MYTHROW(RoutingException,
                ("Wrong position", src.Pos(), "after decoding section", mask, "expected",
                 expectedEndPos, "section size =", section.GetSize()));
      }
    }

    graph.Build(jointIdConverter.GetCount());
  }

private:
  static uint8_t constexpr kVersion = 0;

  class Section final
  {
  public:
    Section() = default;

    Section(VehicleMask mask, uint64_t size, uint32_t numRoads, Joint::Id beginJointId,
            Joint::Id endJointId)
      : m_size(size)
      , m_numRoads(numRoads)
      , m_beginJointId(beginJointId)
      , m_endJointId(endJointId)
      , m_mask(mask)
    {
    }

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_size);
      WriteToSink(sink, m_numRoads);
      WriteToSink(sink, m_beginJointId);
      WriteToSink(sink, m_endJointId);
      WriteToSink(sink, m_mask);
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_size = ReadPrimitiveFromSource<decltype(m_size)>(src);
      m_numRoads = ReadPrimitiveFromSource<decltype(m_numRoads)>(src);
      m_beginJointId = ReadPrimitiveFromSource<decltype(m_beginJointId)>(src);
      m_endJointId = ReadPrimitiveFromSource<decltype(m_endJointId)>(src);
      m_mask = ReadPrimitiveFromSource<decltype(m_mask)>(src);
    }

    uint64_t GetSize() const { return m_size; }
    uint32_t GetNumRoads() const { return m_numRoads; }
    Joint::Id GetBeginJointId() const { return m_beginJointId; }
    Joint::Id GetEndJointId() const { return m_endJointId; }
    VehicleMask GetMask() const { return m_mask; }

  private:
    uint64_t m_size = 0;
    uint32_t m_numRoads = 0;
    Joint::Id m_beginJointId = Joint::kInvalidId;
    Joint::Id m_endJointId = Joint::kInvalidId;
    VehicleMask m_mask = 0;
  };

  class Header final
  {
  public:
    Header() = default;

    Header(IndexGraph const & graph, unordered_map<uint32_t, VehicleMask> const & masks)
      : m_numRoads(graph.GetNumRoads()), m_numJoints(graph.GetNumJoints())
    {
    }

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_numRoads);
      WriteToSink(sink, m_numJoints);

      WriteToSink(sink, static_cast<uint32_t>(m_sections.size()));
      for (Section const & section : m_sections)
        section.Serialize(sink);
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

      auto const sectionsSize = ReadPrimitiveFromSource<uint32_t>(src);
      m_sections.resize(sectionsSize);
      for (Section & section : m_sections)
        section.Deserialize(src);
    }

    uint32_t GetNumRoads() const { return m_numRoads; }
    Joint::Id GetNumJoints() const { return m_numJoints; }
    uint32_t GetNumSections() const { return m_sections.size(); }
    Section const & GetSection(uint32_t index) const
    {
      CHECK_LESS(index, m_sections.size(), ());
      return m_sections[index];
    }

    void AddSection(Section const & section) { m_sections.push_back(section); }
  private:
    uint8_t m_version = kVersion;
    uint32_t m_numRoads = 0;
    Joint::Id m_numJoints = 0;
    vector<Section> m_sections;
  };

  class JointIdEncoder final
  {
  public:
    template <class Sink>
    void Write(Joint::Id originalId, BitWriter<Sink> & writer)
    {
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
    Joint::Id m_count = 0;
    unordered_map<Joint::Id, Joint::Id> m_convertedIds;
  };

  class JointIdDecoder final
  {
  public:
    JointIdDecoder(Joint::Id begin) : m_count(begin) {}
    template <class Source>
    Joint::Id Read(BitReader<Source> & reader)
    {
      uint8_t const repeatBit = reader.Read(1);
      if (repeatBit)
      {
        uint32_t const delta = Read32(reader);
        if (delta > m_count)
          MYTHROW(RoutingException, ("Joint id delta", delta, "> count =", m_count));

        return m_count - delta;
      }

      return m_count++;
    }

    Joint::Id GetCount() const { return m_count; }
  private:
    Joint::Id m_count;
  };

  class JointIdConverter final
  {
  public:
    JointIdConverter(Joint::Id numJoints) { m_ids.assign(numJoints, Joint::kInvalidId); }
    Joint::Id Convert(Joint::Id jointIdInFile)
    {
      if (jointIdInFile >= m_ids.size())
        MYTHROW(RoutingException,
                ("Saved joint id", jointIdInFile, " out of bounds", m_ids.size()));

      Joint::Id & idForMemory = m_ids[jointIdInFile];
      if (idForMemory == Joint::kInvalidId)
      {
        idForMemory = m_count;
        ++m_count;
      }

      return idForMemory;
    }

    Joint::Id GetCount() const { return m_count; }
  private:
    Joint::Id m_count = 0;
    vector<Joint::Id> m_ids;
  };

  class SectionSerializer final
  {
  public:
    explicit SectionSerializer(VehicleMask mask) : m_mask(mask) {}

    uint64_t GetBufferSize() const { return m_buffer.size(); }
    VehicleMask GetMask() const { return m_mask; }
    uint32_t GetNumRoads() const { return m_featureIds.size(); }

    void AddRoad(uint32_t featureId) { m_featureIds.push_back(featureId); }
    void SortRoads() { sort(m_featureIds.begin(), m_featureIds.end()); }
    void SerializeToBuffer(IndexGraph const & graph,
                           unordered_map<uint32_t, VehicleMask> const & masks,
                           JointIdEncoder & jointEncoder);

    template <class Sink>
    void Flush(Sink & sink)
    {
      sink.Write(m_buffer.data(), m_buffer.size());
      m_buffer.clear();
    }

  private:
    VehicleMask const m_mask;
    vector<uint32_t> m_featureIds;
    vector<uint8_t> m_buffer;
  };

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

  static VehicleMask GetRoadMask(unordered_map<uint32_t, VehicleMask> const & masks,
                                 uint32_t featureId);
  static VehicleMask GetJointMask(IndexGraph const & graph,
                                  unordered_map<uint32_t, VehicleMask> const & masks,
                                  Joint::Id jointId);
  static void PrepareSectionSerializers(IndexGraph const & graph,
                                        unordered_map<uint32_t, VehicleMask> const & masks,
                                        vector<SectionSerializer> & sections);
};
}  // namespace routing
