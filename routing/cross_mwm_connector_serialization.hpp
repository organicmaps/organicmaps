#pragma once

#include "routing/coding.hpp"
#include "routing/cross_mwm_connector.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/transit_types.hpp"

#include "indexer/coding_params.hpp"
#include "indexer/geometry_serialization.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/checked_cast.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace routing
{
template <typename CrossMwmId>
using CrossMwmConnectorPerVehicleType =
    std::array<CrossMwmConnector<CrossMwmId>, static_cast<size_t>(VehicleType::Count)>;

class CrossMwmConnectorSerializer final
{
public:
  static uint8_t constexpr stopIdBits = 64;
  static uint8_t constexpr lineIdBits = 32;

  template <typename CrossMwmId>
  class Transition final
  {
  public:
    Transition() = default;

    Transition(CrossMwmId const & crossMwmId, uint32_t featureId, uint32_t segmentIdx,
               VehicleMask roadMask, VehicleMask oneWayMask, bool forwardIsEnter,
               m2::PointD const & backPoint, m2::PointD const & frontPoint)
      : m_crossMwmId(crossMwmId)
      , m_featureId(featureId)
      , m_segmentIdx(segmentIdx)
      , m_backPoint(backPoint)
      , m_frontPoint(frontPoint)
      , m_roadMask(roadMask)
      , m_oneWayMask(oneWayMask)
      , m_forwardIsEnter(forwardIsEnter)
    {
    }

    template <class Sink>
    void WriteCrossMwmId(connector::OsmId const & id, uint8_t n, BitWriter<Sink> & w) const
    {
      w.WriteAtMost64Bits(id, n);
    }

    template <class Sink>
    void WriteCrossMwmId(connector::TransitId const & id, uint8_t /* n */, BitWriter<Sink> & w) const
    {
      w.WriteAtMost64Bits(id.m_stop1Id, stopIdBits);
      w.WriteAtMost64Bits(id.m_stop2Id, stopIdBits);
      w.WriteAtMost32Bits(id.m_lineId, lineIdBits);
    }

    template <class Sink>
    void Serialize(serial::CodingParams const & codingParams, uint32_t bitsPerCrossMwmId,
                   uint8_t bitsPerMask, Sink & sink) const
    {
      serial::SavePoint(sink, m_backPoint, codingParams);
      serial::SavePoint(sink, m_frontPoint, codingParams);

      BitWriter<Sink> writer(sink);
      WriteCrossMwmId(m_crossMwmId, bitsPerCrossMwmId, writer);
      WriteDelta(writer, m_featureId + 1);
      WriteDelta(writer, m_segmentIdx + 1);
      writer.WriteAtMost32Bits(base::asserted_cast<uint32_t>(m_roadMask), bitsPerMask);
      writer.WriteAtMost32Bits(base::asserted_cast<uint32_t>(m_oneWayMask), bitsPerMask);
      writer.Write(m_forwardIsEnter ? 0 : 1, 1);
    }

    template <class Source>
    void ReadCrossMwmId(uint8_t /* n */, BitReader<Source> & reader, connector::TransitId & readed)
    {
      readed.m_stop1Id = reader.ReadAtMost64Bits(stopIdBits);
      readed.m_stop2Id = reader.ReadAtMost64Bits(stopIdBits);
      readed.m_lineId = reader.ReadAtMost32Bits(lineIdBits);
    };

    template <class Source>
    void ReadCrossMwmId(uint8_t n, BitReader<Source> & reader, connector::OsmId & readed)
    {
      readed = reader.ReadAtMost64Bits(n);
    }

    template <class Source>
    void Deserialize(serial::CodingParams const & codingParams, uint32_t bitsPerOsmId,
                     uint8_t bitsPerMask, Source & src)
    {
      m_backPoint = serial::LoadPoint(src, codingParams);
      m_frontPoint = serial::LoadPoint(src, codingParams);

      BitReader<Source> reader(src);
      ReadCrossMwmId(bitsPerOsmId, reader, m_crossMwmId);
      m_featureId = ReadDelta<decltype(m_featureId)>(reader) - 1;
      m_segmentIdx = ReadDelta<decltype(m_segmentIdx)>(reader) - 1;
      m_roadMask = base::asserted_cast<VehicleMask>(reader.ReadAtMost32Bits(bitsPerMask));
      m_oneWayMask = base::asserted_cast<VehicleMask>(reader.ReadAtMost32Bits(bitsPerMask));
      m_forwardIsEnter = reader.Read(1) == 0;
    }

    CrossMwmId const & GetCrossMwmId() const { return m_crossMwmId; }
    uint32_t GetFeatureId() const { return m_featureId; }
    uint32_t GetSegmentIdx() const { return m_segmentIdx; }
    m2::PointD const & GetBackPoint() const { return m_backPoint; }
    m2::PointD const & GetFrontPoint() const { return m_frontPoint; }
    bool ForwardIsEnter() const { return m_forwardIsEnter; }
    VehicleMask GetRoadMask() const { return m_roadMask; }
    VehicleMask GetOneWayMask() const { return m_oneWayMask; }

  private:
    CrossMwmId m_crossMwmId = 0;
    uint32_t m_featureId = 0;
    uint32_t m_segmentIdx = 0;
    m2::PointD m_backPoint = m2::PointD::Zero();
    m2::PointD m_frontPoint = m2::PointD::Zero();
    VehicleMask m_roadMask = 0;
    VehicleMask m_oneWayMask = 0;
    bool m_forwardIsEnter = false;
  };

  CrossMwmConnectorSerializer() = delete;

  template <class Sink, class CrossMwmId>
  static void Serialize(std::vector<Transition<CrossMwmId>> const & transitions,
                        CrossMwmConnectorPerVehicleType<CrossMwmId> const & connectors,
                        serial::CodingParams const & codingParams, Sink & sink)
  {
    uint32_t const bitsPerCrossMwmId = CalcBitsPerCrossMwmId(transitions);
    auto const bitsPerMask = GetBitsPerMask<uint8_t>();
    std::vector<uint8_t> transitionsBuf;
    WriteTransitions(transitions, codingParams, bitsPerCrossMwmId, bitsPerMask, transitionsBuf);

    Header header(base::checked_cast<uint32_t>(transitions.size()),
                  base::checked_cast<uint64_t>(transitionsBuf.size()), codingParams,
                  bitsPerCrossMwmId, bitsPerMask);
    std::vector<std::vector<uint8_t>> weightBuffers(connectors.size());

    for (size_t i = 0; i < connectors.size(); ++i)
    {
      CrossMwmConnector<CrossMwmId> const & connector = connectors[i];
      if (connector.m_weights.empty())
        continue;

      std::vector<uint8_t> & buffer = weightBuffers[i];
      WriteWeights(connector.m_weights, buffer);
      auto const numEnters = base::checked_cast<uint32_t>(connector.GetEnters().size());
      auto const numExits = base::checked_cast<uint32_t>(connector.GetExits().size());
      auto const vehicleType = static_cast<VehicleType>(i);
      header.AddSection(Section(buffer.size(), numEnters, numExits, vehicleType));
    }

    header.Serialize(sink);
    FlushBuffer(transitionsBuf, sink);

    for (auto & buffer : weightBuffers)
      FlushBuffer(buffer, sink);
  }

  template <class Source, class CrossMwmId>
  static void DeserializeTransitions(VehicleType requiredVehicle, CrossMwmConnector<CrossMwmId> & connector,
                                     Source & src)
  {
    CHECK(connector.m_weightsLoadState == connector::WeightsLoadState::Unknown, ());

    Header header;
    header.Deserialize(src);

    uint64_t weightsOffset = src.Pos() + header.GetSizeTransitions();
    VehicleMask const requiredMask = GetVehicleMask(requiredVehicle);
    auto const numTransitions = base::checked_cast<size_t>(header.GetNumTransitions());

    for (size_t i = 0; i < numTransitions; ++i)
    {
      Transition<CrossMwmId> transition;
      transition.Deserialize(header.GetCodingParams(), header.GetBitsPerOsmId(),
                             header.GetBitsPerMask(), src);
      AddTransition(transition, requiredMask, connector);
    }

    if (src.Pos() != weightsOffset)
    {
      MYTHROW(CorruptedDataException,
              ("Wrong position", src.Pos(), "after decoding transitions, expected:",
               connector.m_weightsOffset, "size:", header.GetSizeTransitions()));
    }

    for (Section const & section : header.GetSections())
    {
      if (section.GetVehicleType() != requiredVehicle)
      {
        weightsOffset += section.GetSize();
        continue;
      }

      size_t const numEnters = connector.GetEnters().size();
      size_t const numExits = connector.GetExits().size();

      if (base::checked_cast<size_t>(section.GetNumEnters()) != numEnters)
      {
        MYTHROW(CorruptedDataException, ("Mismatch enters number, section:", section.GetNumEnters(),
                                         ", connector:", numEnters));
      }

      if (base::checked_cast<size_t>(section.GetNumExits()) != numExits)
      {
        MYTHROW(CorruptedDataException, ("Mismatch exits number, section:", section.GetNumExits(),
                                         ", connector:", numExits));
      }

      connector.m_weightsOffset = weightsOffset;
      connector.m_granularity = header.GetGranularity();
      connector.m_weightsLoadState = connector::WeightsLoadState::ReadyToLoad;
      return;
    }

    connector.m_weightsLoadState = connector::WeightsLoadState::NotExists;
  }

  template <class Source, class CrossMwmId>
  static void DeserializeWeights(VehicleType /* vehicle */, CrossMwmConnector<CrossMwmId> & connector,
                                 Source & src)
  {
    CHECK(connector.m_weightsLoadState == connector::WeightsLoadState::ReadyToLoad, ());
    CHECK_GREATER(connector.m_granularity, 0, ());

    src.Skip(connector.m_weightsOffset);

    size_t const amount = connector.GetEnters().size() * connector.GetExits().size();
    connector.m_weights.reserve(amount);

    BitReader<Source> reader(src);

    Weight prev = 1;
    for (size_t i = 0; i < amount; ++i)
    {
      if (reader.Read(1) == kNoRouteBit)
      {
        connector.m_weights.push_back(connector::kNoRoute);
        continue;
      }

      Weight const delta = ReadDelta<Weight>(reader) - 1;
      Weight const current = DecodeZigZagDelta(prev, delta);
      connector.m_weights.push_back(current * connector.m_granularity);
      prev = current;
    }

    connector.m_weightsLoadState = connector::WeightsLoadState::Loaded;
  }

  template <class CrossMwmId>
  static void AddTransition(Transition<CrossMwmId> const & transition, VehicleMask requiredMask,
                            CrossMwmConnector<CrossMwmId> & connector)
  {
    if ((transition.GetRoadMask() & requiredMask) == 0)
      return;

    bool const isOneWay = (transition.GetOneWayMask() & requiredMask) != 0;
    connector.AddTransition(transition.GetCrossMwmId(), transition.GetFeatureId(),
                            transition.GetSegmentIdx(), isOneWay, transition.ForwardIsEnter(),
                            transition.GetBackPoint(), transition.GetFrontPoint());
  }

private:
  using Weight = connector::Weight;
  using WeightsLoadState = connector::WeightsLoadState;

  static uint32_t constexpr kLastVersion = 0;
  static uint8_t constexpr kNoRouteBit = 0;
  static uint8_t constexpr kRouteBit = 1;

  // Weight is rounded up to the next multiple of kGranularity before being stored in the section.
  // kGranularity is measured in seconds as well as Weight.
  // Increasing kGranularity will decrease the section's size.
  static Weight constexpr kGranularity = 4;
  static_assert(kGranularity > 0, "kGranularity should be > 0");

  class Section final
  {
  public:
    Section() = default;

    Section(uint64_t size, uint32_t numEnters, uint32_t numExits, VehicleType vehicleType)
      : m_size(size), m_numEnters(numEnters), m_numExits(numExits), m_vehicleType(vehicleType)
    {
    }

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_size);
      WriteToSink(sink, m_numEnters);
      WriteToSink(sink, m_numExits);
      WriteToSink(sink, static_cast<uint8_t>(m_vehicleType));
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_size = ReadPrimitiveFromSource<decltype(m_size)>(src);
      m_numEnters = ReadPrimitiveFromSource<decltype(m_numEnters)>(src);
      m_numExits = ReadPrimitiveFromSource<decltype(m_numExits)>(src);
      m_vehicleType = static_cast<VehicleType>(ReadPrimitiveFromSource<uint8_t>(src));
    }

    uint64_t GetSize() const { return m_size; }
    uint32_t GetNumEnters() const { return m_numEnters; }
    uint32_t GetNumExits() const { return m_numExits; }
    VehicleType GetVehicleType() const { return m_vehicleType; }

  private:
    uint64_t m_size = 0;
    uint32_t m_numEnters = 0;
    uint32_t m_numExits = 0;
    VehicleType m_vehicleType = VehicleType::Pedestrian;
  };

  // @todo(bykoianko) It's necessary to consider implementation Header template class.
  // For different cross mwm id may be needs different headers. For example without |bitsPerOsmId|.
  class Header final
  {
  public:
    Header() = default;

    Header(uint32_t numTransitions, uint64_t sizeTransitions,
           serial::CodingParams const & codingParams, uint32_t bitsPerCrossMwmId, uint8_t bitsPerMask)
      : m_numTransitions(numTransitions)
      , m_sizeTransitions(sizeTransitions)
      , m_codingParams(codingParams)
      , m_bitsPerCrossMwmId(bitsPerCrossMwmId)
      , m_bitsPerMask(bitsPerMask)
    {
    }

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_numTransitions);
      WriteToSink(sink, m_sizeTransitions);
      WriteToSink(sink, m_granularity);
      m_codingParams.Save(sink);
      WriteToSink(sink, m_bitsPerCrossMwmId);
      WriteToSink(sink, m_bitsPerMask);

      WriteToSink(sink, base::checked_cast<uint32_t>(m_sections.size()));
      for (Section const & section : m_sections)
        section.Serialize(sink);
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_version = ReadPrimitiveFromSource<decltype(m_version)>(src);
      if (m_version != kLastVersion)
      {
        MYTHROW(CorruptedDataException, ("Unknown cross mwm section version ", m_version,
                                         ", current version ", kLastVersion));
      }

      m_numTransitions = ReadPrimitiveFromSource<decltype(m_numTransitions)>(src);
      m_sizeTransitions = ReadPrimitiveFromSource<decltype(m_sizeTransitions)>(src);
      m_granularity = ReadPrimitiveFromSource<decltype(m_granularity)>(src);
      m_codingParams.Load(src);
      m_bitsPerCrossMwmId = ReadPrimitiveFromSource<decltype(m_bitsPerCrossMwmId)>(src);
      m_bitsPerMask = ReadPrimitiveFromSource<decltype(m_bitsPerMask)>(src);

      auto const sectionsSize = ReadPrimitiveFromSource<uint32_t>(src);
      m_sections.resize(base::checked_cast<size_t>(sectionsSize));
      for (Section & section : m_sections)
        section.Deserialize(src);
    }

    void AddSection(Section const & section) { m_sections.push_back(section); }

    uint32_t GetNumTransitions() const { return m_numTransitions; }
    uint64_t GetSizeTransitions() const { return m_sizeTransitions; }
    Weight GetGranularity() const { return m_granularity; }
    serial::CodingParams const & GetCodingParams() const { return m_codingParams; }
    uint8_t GetBitsPerOsmId() const { return m_bitsPerCrossMwmId; }
    uint8_t GetBitsPerMask() const { return m_bitsPerMask; }
    std::vector<Section> const & GetSections() const { return m_sections; }

  private:
    uint32_t m_version = kLastVersion;
    uint32_t m_numTransitions = 0;
    uint64_t m_sizeTransitions = 0;
    Weight m_granularity = kGranularity;
    serial::CodingParams m_codingParams;
    uint32_t m_bitsPerCrossMwmId = 0;
    uint8_t m_bitsPerMask = 0;
    std::vector<Section> m_sections;
  };

  static uint32_t CalcBitsPerCrossMwmId(
      std::vector<Transition<connector::OsmId>> const & transitions)
  {
    connector::OsmId osmId = 0;
    for (Transition<connector::OsmId> const & transition : transitions)
      osmId = std::max(osmId, transition.GetCrossMwmId());

    return bits::NumUsedBits(osmId);
  }

  static uint32_t CalcBitsPerCrossMwmId(
      std::vector<Transition<connector::TransitId>> const & /* transitions */)
  {
    // Bit per osm id is not actual for TransitId case.
    return 0;
  }

  template <typename T>
  static T GetBitsPerMask()
  {
    static_assert(
        static_cast<size_t>(VehicleType::Count) <= static_cast<size_t>(numeric_limits<T>::max()),
        "Can't pack VehicleType::Count into chosen type");
    return static_cast<T>(VehicleType::Count);
  }

  template <class Sink>
  static void FlushBuffer(std::vector<uint8_t> & buffer, Sink & sink)
  {
    sink.Write(buffer.data(), buffer.size());
    buffer.clear();
  }

  template <typename CrossMwmId>
  static void WriteTransitions(std::vector<Transition<CrossMwmId>> const & transitions,
                               serial::CodingParams const & codingParams, uint32_t bitsPerOsmId,
                               uint8_t bitsPerMask, std::vector<uint8_t> & buffer)
  {
    MemWriter<vector<uint8_t>> memWriter(buffer);

    for (auto const & transition : transitions)
      transition.Serialize(codingParams, bitsPerOsmId, bitsPerMask, memWriter);
  }

  static void WriteWeights(std::vector<Weight> const & weights, std::vector<uint8_t> & buffer);
};

static_assert(sizeof(transit::StopId) * 8 == 64, "Wrong transit::StopId size");
static_assert(sizeof(transit::LineId) * 8 == 32, "Wrong transit::LineId size");
}  // namespace routing
