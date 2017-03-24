#pragma once

#include "routing/coding.hpp"
#include "routing/cross_mwm_connector.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/vehicle_mask.hpp"

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
using CrossMwmConnectorPerVehicleType =
    std::array<CrossMwmConnector, static_cast<size_t>(VehicleType::Count)>;

class CrossMwmConnectorSerializer final
{
public:
  class Transition final
  {
  public:
    Transition() = default;

    Transition(uint64_t osmId, uint32_t featureId, uint32_t segmentIdx, VehicleMask roadMask,
               VehicleMask oneWayMask, bool forwardIsEnter, m2::PointD const & backPoint,
               m2::PointD const & frontPoint)
      : m_osmId(osmId)
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
    void Serialize(serial::CodingParams const & codingParams, uint32_t bitsPerOsmId,
                   uint8_t bitsPerMask, Sink & sink) const
    {
      serial::SavePoint(sink, m_backPoint, codingParams);
      serial::SavePoint(sink, m_frontPoint, codingParams);

      BitWriter<Sink> writer(sink);
      writer.WriteAtMost64Bits(m_osmId, bitsPerOsmId);
      WriteDelta(writer, m_featureId + 1);
      WriteDelta(writer, m_segmentIdx + 1);
      writer.WriteAtMost32Bits(base::asserted_cast<uint32_t>(m_roadMask), bitsPerMask);
      writer.WriteAtMost32Bits(base::asserted_cast<uint32_t>(m_oneWayMask), bitsPerMask);
      writer.Write(m_forwardIsEnter ? 0 : 1, 1);
    }

    template <class Source>
    void Deserialize(serial::CodingParams const & codingParams, uint32_t bitsPerOsmId,
                     uint8_t bitsPerMask, Source & src)
    {
      m_backPoint = serial::LoadPoint(src, codingParams);
      m_frontPoint = serial::LoadPoint(src, codingParams);

      BitReader<Source> reader(src);
      m_osmId = reader.ReadAtMost64Bits(bitsPerOsmId);
      m_featureId = ReadDelta<decltype(m_featureId)>(reader) - 1;
      m_segmentIdx = ReadDelta<decltype(m_segmentIdx)>(reader) - 1;
      m_roadMask = base::asserted_cast<VehicleMask>(reader.ReadAtMost32Bits(bitsPerMask));
      m_oneWayMask = base::asserted_cast<VehicleMask>(reader.ReadAtMost32Bits(bitsPerMask));
      m_forwardIsEnter = reader.Read(1) == 0;
    }

    uint64_t GetOsmId() const { return m_osmId; }
    uint32_t GetFeatureId() const { return m_featureId; }
    uint32_t GetSegmentIdx() const { return m_segmentIdx; }
    m2::PointD const & GetBackPoint() const { return m_backPoint; }
    m2::PointD const & GetFrontPoint() const { return m_frontPoint; }
    bool ForwardIsEnter() const { return m_forwardIsEnter; }
    VehicleMask GetRoadMask() const { return m_roadMask; }
    VehicleMask GetOneWayMask() const { return m_oneWayMask; }

  private:
    uint64_t m_osmId = 0;
    uint32_t m_featureId = 0;
    uint32_t m_segmentIdx = 0;
    m2::PointD m_backPoint = m2::PointD::Zero();
    m2::PointD m_frontPoint = m2::PointD::Zero();
    VehicleMask m_roadMask = 0;
    VehicleMask m_oneWayMask = 0;
    bool m_forwardIsEnter = false;
  };

  CrossMwmConnectorSerializer() = delete;

  template <class Sink>
  static void Serialize(std::vector<Transition> const & transitions,
                        CrossMwmConnectorPerVehicleType const & connectors,
                        serial::CodingParams const & codingParams, Sink & sink)
  {
    uint32_t const bitsPerOsmId = CalcBitsPerOsmId(transitions);
    auto const bitsPerMask = GetBitsPerMask<uint8_t>();
    std::vector<uint8_t> transitionsBuf;
    WriteTransitions(transitions, codingParams, bitsPerOsmId, bitsPerMask, transitionsBuf);

    Header header(base::checked_cast<uint32_t>(transitions.size()),
                  base::checked_cast<uint64_t>(transitionsBuf.size()), codingParams, bitsPerOsmId,
                  bitsPerMask);
    std::vector<std::vector<uint8_t>> weightBuffers(connectors.size());

    for (size_t i = 0; i < connectors.size(); ++i)
    {
      CrossMwmConnector const & connector = connectors[i];
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

  template <class Source>
  static void DeserializeTransitions(VehicleType requiredVehicle, CrossMwmConnector & connector,
                                     Source & src)
  {
    CHECK(connector.m_weightsLoadState == WeightsLoadState::Unknown, ());

    Header header;
    header.Deserialize(src);

    uint64_t weightsOffset = src.Pos() + header.GetSizeTransitions();
    VehicleMask const requiredMask = GetVehicleMask(requiredVehicle);
    auto const numTransitions = base::checked_cast<size_t>(header.GetNumTransitions());

    for (size_t i = 0; i < numTransitions; ++i)
    {
      Transition transition;
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
      connector.m_weightsLoadState = WeightsLoadState::ReadyToLoad;
      return;
    }

    connector.m_weightsLoadState = WeightsLoadState::NotExists;
  }

  template <class Source>
  static void DeserializeWeights(VehicleType /* vehicle */, CrossMwmConnector & connector,
                                 Source & src)
  {
    CHECK(connector.m_weightsLoadState == WeightsLoadState::ReadyToLoad, ());
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
        connector.m_weights.push_back(CrossMwmConnector::kNoRoute);
        continue;
      }

      Weight const delta = ReadDelta<Weight>(reader) - 1;
      Weight const current = DecodeZigZagDelta(prev, delta);
      connector.m_weights.push_back(current * connector.m_granularity);
      prev = current;
    }

    connector.m_weightsLoadState = WeightsLoadState::Loaded;
  }

  static void AddTransition(Transition const & transition, VehicleMask requiredMask,
                            CrossMwmConnector & connector)
  {
    if ((transition.GetRoadMask() & requiredMask) == 0)
      return;

    bool const isOneWay = (transition.GetOneWayMask() & requiredMask) != 0;
    connector.AddTransition(transition.GetOsmId(), transition.GetFeatureId(),
                            transition.GetSegmentIdx(), isOneWay, transition.ForwardIsEnter(),
                            transition.GetBackPoint(), transition.GetFrontPoint());
  }

private:
  using Weight = CrossMwmConnector::Weight;
  using WeightsLoadState = CrossMwmConnector::WeightsLoadState;

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

  class Header final
  {
  public:
    Header() = default;

    Header(uint32_t numTransitions, uint64_t sizeTransitions,
           serial::CodingParams const & codingParams, uint32_t bitsPerOsmId, uint8_t bitsPerMask)
      : m_numTransitions(numTransitions)
      , m_sizeTransitions(sizeTransitions)
      , m_codingParams(codingParams)
      , m_bitsPerOsmId(bitsPerOsmId)
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
      WriteToSink(sink, m_bitsPerOsmId);
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
      m_bitsPerOsmId = ReadPrimitiveFromSource<decltype(m_bitsPerOsmId)>(src);
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
    uint8_t GetBitsPerOsmId() const { return m_bitsPerOsmId; }
    uint8_t GetBitsPerMask() const { return m_bitsPerMask; }
    std::vector<Section> const & GetSections() const { return m_sections; }

  private:
    uint32_t m_version = kLastVersion;
    uint32_t m_numTransitions = 0;
    uint64_t m_sizeTransitions = 0;
    Weight m_granularity = kGranularity;
    serial::CodingParams m_codingParams;
    uint32_t m_bitsPerOsmId = 0;
    uint8_t m_bitsPerMask = 0;
    std::vector<Section> m_sections;
  };

  static uint32_t CalcBitsPerOsmId(std::vector<Transition> const & transitions);

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

  static void WriteTransitions(std::vector<Transition> const & transitions,
                               serial::CodingParams const & codingParams, uint32_t bitsPerOsmId,
                               uint8_t bitsPerMask, std::vector<uint8_t> & buffer);

  static void WriteWeights(std::vector<Weight> const & weights, std::vector<uint8_t> & buffer);
};
}  // namespace routing
