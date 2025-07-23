#pragma once

#include "routing/coding.hpp"
#include "routing/cross_mwm_connector.hpp"
#include "routing/cross_mwm_ids.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/vehicle_mask.hpp"

#include "transit/transit_types.hpp"

#include "coding/bit_streams.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/checked_cast.hpp"
#include "base/geo_object_id.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace routing
{
namespace connector
{
static uint8_t constexpr kOsmIdBits = 64;
static uint8_t constexpr kStopIdBits = 64;
static uint8_t constexpr kLineIdBits = 32;

inline uint32_t constexpr CalcBitsPerTransitId()
{
  return 2 * kStopIdBits + kLineIdBits;
}

template <typename CrossMwmId>
uint32_t constexpr GetFeaturesOffset() noexcept;
template <>
uint32_t constexpr GetFeaturesOffset<base::GeoObjectId>() noexcept
{
  return 0;
}
template <>
uint32_t constexpr GetFeaturesOffset<TransitId>() noexcept
{
  return FakeFeatureIds::kTransitGraphFeaturesStart;
}
}  // namespace connector

/// Builder class for deserialization.
template <typename CrossMwmId>
class CrossMwmConnectorBuilder
{
protected:
  using ConnectorT = CrossMwmConnector<CrossMwmId>;
  ConnectorT & m_c;

  // For some connectors we may need to shift features with some offset.
  // For example for versions and transit section compatibility we number transit features
  // starting from 0 in mwm and shift them with |m_featureNumerationOffset| in runtime.
  uint32_t m_featureNumerationOffset = 0;

public:
  explicit CrossMwmConnectorBuilder(ConnectorT & c) : m_c(c) {}

  /// Called only in cross-mwm-graph when deserialization connectors.
  void ApplyNumerationOffset() { m_featureNumerationOffset = connector::GetFeaturesOffset<CrossMwmId>(); }

  void AddTransition(CrossMwmId const & crossMwmId, uint32_t featureId, uint32_t segmentIdx, bool oneWay,
                     bool forwardIsEnter)
  {
    featureId += m_featureNumerationOffset;

    typename ConnectorT::Transition transition(m_c.m_entersCount, m_c.m_exitsCount, crossMwmId, oneWay, forwardIsEnter);

    if (forwardIsEnter)
      ++m_c.m_entersCount;
    else
      ++m_c.m_exitsCount;

    if (!oneWay)
    {
      if (forwardIsEnter)
        ++m_c.m_exitsCount;
      else
        ++m_c.m_entersCount;
    }

    m_c.m_transitions.emplace_back(typename ConnectorT::Key(featureId, segmentIdx), transition);
    m_c.m_crossMwmIdToFeatureId.emplace(crossMwmId, featureId);
  }

protected:
  template <class GetTransition>
  void FillTransitions(size_t count, VehicleType requiredVehicle, GetTransition && getter)
  {
    auto const vhMask = GetVehicleMask(requiredVehicle);

    m_c.m_transitions.reserve(count);
    for (size_t i = 0; i < count; ++i)
      AddTransition(getter(i), vhMask);

    // Sort by FeatureID to make lower_bound queries.
    std::sort(m_c.m_transitions.begin(), m_c.m_transitions.end(), typename ConnectorT::LessKT());
  }

  class Transition final
  {
  public:
    Transition() = default;

    Transition(CrossMwmId const & crossMwmId, uint32_t featureId, uint32_t segmentIdx, VehicleMask roadMask,
               VehicleMask oneWayMask, bool forwardIsEnter)
      : m_crossMwmId(crossMwmId)
      , m_featureId(featureId)
      , m_segmentIdx(segmentIdx)
      , m_roadMask(roadMask)
      , m_oneWayMask(oneWayMask)
      , m_forwardIsEnter(forwardIsEnter)
    {}

    template <class Sink>
    static void WriteCrossMwmId(base::GeoObjectId const & id, uint8_t bits, BitWriter<Sink> & w)
    {
      CHECK_LESS_OR_EQUAL(bits, connector::kOsmIdBits, ());
      w.WriteAtMost64Bits(id.GetEncodedId(), bits);
    }

    template <class Sink>
    static void WriteCrossMwmId(connector::TransitId const & id, uint8_t bitsPerCrossMwmId, BitWriter<Sink> & w)
    {
      CHECK_EQUAL(bitsPerCrossMwmId, connector::CalcBitsPerTransitId(), ("Wrong TransitId size."));
      w.WriteAtMost64Bits(id.m_stop1Id, connector::kStopIdBits);
      w.WriteAtMost64Bits(id.m_stop2Id, connector::kStopIdBits);
      w.WriteAtMost32Bits(id.m_lineId, connector::kLineIdBits);
    }

    template <class Sink>
    void Serialize(uint32_t bitsPerCrossMwmId, uint8_t bitsPerMask, Sink & sink) const
    {
      // TODO (@gmoryes):
      //  We do not use back and front point of segment in code, so we just write
      //  zero to this 128 bits. Need to remove this data from mwm section.
      WriteVarUint<uint64_t>(sink, 0);
      WriteVarUint<uint64_t>(sink, 0);

      BitWriter<Sink> writer(sink);
      WriteCrossMwmId(m_crossMwmId, bitsPerCrossMwmId, writer);
      WriteDelta(writer, m_featureId + 1);
      WriteDelta(writer, m_segmentIdx + 1);
      writer.WriteAtMost32Bits(base::asserted_cast<uint32_t>(m_roadMask), bitsPerMask);
      writer.WriteAtMost32Bits(base::asserted_cast<uint32_t>(m_oneWayMask), bitsPerMask);
      writer.Write(m_forwardIsEnter ? 0 : 1, 1);
    }

    template <class Source>
    static void ReadCrossMwmId(uint8_t bitsPerCrossMwmId, BitReader<Source> & reader, connector::TransitId & readed)
    {
      CHECK_EQUAL(bitsPerCrossMwmId, connector::CalcBitsPerTransitId(), ("Wrong TransitId size."));
      readed.m_stop1Id = reader.ReadAtMost64Bits(connector::kStopIdBits);
      readed.m_stop2Id = reader.ReadAtMost64Bits(connector::kStopIdBits);
      readed.m_lineId = reader.ReadAtMost32Bits(connector::kLineIdBits);
    }

    template <class Source>
    static void ReadCrossMwmId(uint8_t bits, BitReader<Source> & reader, base::GeoObjectId & osmId)
    {
      CHECK_LESS_OR_EQUAL(bits, connector::kOsmIdBits, ());
      // We lost data about transition type after compression (look at CalcBitsPerCrossMwmId method)
      // but we used Way in routing, so suggest that it is Osm Way.
      osmId = base::MakeOsmWay(reader.ReadAtMost64Bits(bits));
    }

    template <class Source>
    void Deserialize(uint32_t bitsPerCrossMwmId, uint8_t bitsPerMask, Source & src)
    {
      // TODO (@gmoryes)
      //  We do not use back and front point of segment in code, so we just skip this 128 bits.
      //  Need to remove this data from mwm section.
      UNUSED_VALUE(ReadVarUint<uint64_t>(src));
      UNUSED_VALUE(ReadVarUint<uint64_t>(src));

      BitReader<Source> reader(src);
      ReadCrossMwmId(bitsPerCrossMwmId, reader, m_crossMwmId);
      m_featureId = ReadDelta<decltype(m_featureId)>(reader) - 1;
      m_segmentIdx = ReadDelta<decltype(m_segmentIdx)>(reader) - 1;
      m_roadMask = base::asserted_cast<VehicleMask>(reader.ReadAtMost32Bits(bitsPerMask));
      m_oneWayMask = base::asserted_cast<VehicleMask>(reader.ReadAtMost32Bits(bitsPerMask));
      m_forwardIsEnter = reader.Read(1) == 0;
    }

    CrossMwmId const & GetCrossMwmId() const { return m_crossMwmId; }
    uint32_t GetFeatureId() const { return m_featureId; }
    uint32_t GetSegmentIdx() const { return m_segmentIdx; }
    bool ForwardIsEnter() const { return m_forwardIsEnter; }
    VehicleMask GetRoadMask() const { return m_roadMask; }
    VehicleMask GetOneWayMask() const { return m_oneWayMask; }

  private:
    CrossMwmId m_crossMwmId{};
    uint32_t m_featureId = 0;
    uint32_t m_segmentIdx = 0;
    VehicleMask m_roadMask = 0;
    VehicleMask m_oneWayMask = 0;
    bool m_forwardIsEnter = false;
  };

  void CheckBitsPerCrossMwmId(uint32_t bitsPerCrossMwmId) const;

public:
  void DeserializeTransitions(VehicleType requiredVehicle, FilesContainerR::TReader & reader)
  {
    DeserializeTransitions(requiredVehicle, *(reader.GetPtr()));
  }

  template <class Reader>
  void DeserializeTransitions(VehicleType requiredVehicle, Reader & reader)
  {
    CHECK(m_c.m_weights.m_loadState == connector::WeightsLoadState::Unknown, ());

    NonOwningReaderSource src(reader);
    Header header;
    header.Deserialize(src);
    CheckBitsPerCrossMwmId(header.GetBitsPerCrossMwmId());

    uint64_t weightsOffset = src.Pos() + header.GetSizeTransitions();
    auto const numTransitions = base::checked_cast<size_t>(header.GetNumTransitions());

    FillTransitions(numTransitions, requiredVehicle, [&header, &src](size_t)
    {
      Transition transition;
      transition.Deserialize(header.GetBitsPerCrossMwmId(), header.GetBitsPerMask(), src);
      return transition;
    });

    if (src.Pos() != weightsOffset)
    {
      MYTHROW(CorruptedDataException, ("Wrong position", src.Pos(), "after decoding transitions, expected:",
                                       weightsOffset, "size:", header.GetSizeTransitions()));
    }

    for (Section const & section : header.GetSections())
    {
      if (section.GetVehicleType() != requiredVehicle)
      {
        weightsOffset += section.GetSize();
        continue;
      }

      size_t const numEnters = m_c.GetNumEnters();
      size_t const numExits = m_c.GetNumExits();

      if (base::checked_cast<size_t>(section.GetNumEnters()) != numEnters)
      {
        MYTHROW(CorruptedDataException,
                ("Mismatch enters number, section:", section.GetNumEnters(), ", connector:", numEnters));
      }

      if (base::checked_cast<size_t>(section.GetNumExits()) != numExits)
      {
        MYTHROW(CorruptedDataException,
                ("Mismatch exits number, section:", section.GetNumExits(), ", connector:", numExits));
      }

      m_c.m_weights.m_offset = weightsOffset;
      m_c.m_weights.m_granularity = header.GetGranularity();
      m_c.m_weights.m_version = header.GetVersion();
      m_c.m_weights.m_loadState = connector::WeightsLoadState::ReadyToLoad;
      return;
    }

    m_c.m_weights.m_loadState = connector::WeightsLoadState::NotExists;
  }

  void DeserializeWeights(FilesContainerR::TReader & reader) { DeserializeWeights(*(reader.GetPtr())); }

  /// @param[in]  reader  Initialized reader for the whole section (makes Skip inside).
  template <class Reader>
  void DeserializeWeights(Reader & reader)
  {
    CHECK(m_c.m_weights.m_loadState == connector::WeightsLoadState::ReadyToLoad, ());
    CHECK_GREATER(m_c.m_weights.m_granularity, 0, ());

    size_t const amount = m_c.GetNumEnters() * m_c.GetNumExits();

    if (m_c.m_weights.m_version < 2)
    {
      NonOwningReaderSource src(reader);
      src.Skip(m_c.m_weights.m_offset);

      // Do reserve memory here to avoid a lot of reallocations.
      // SparseVector will shrink final vector if needed.
      coding::SparseVectorBuilder<Weight> builder(amount);
      BitReader bitReader(src);

      Weight prev = 1;
      for (size_t i = 0; i < amount; ++i)
        if (bitReader.Read(1) != kNoRouteBit)
        {
          Weight const delta = ReadDelta<Weight>(bitReader) - 1;
          Weight const current = DecodeZigZagDelta(prev, delta);
          builder.PushValue(current * m_c.m_weights.m_granularity);
          prev = current;
        }
        else
          builder.PushEmpty();

      m_c.m_weights.m_v1 = builder.Build();
    }
    else
    {
      m_c.m_weights.m_reader = reader.CreateSubReader(m_c.m_weights.m_offset, reader.Size() - m_c.m_weights.m_offset);
      m_c.m_weights.m_v2 = MapUint32ToValue<Weight>::Load(
          *(m_c.m_weights.m_reader),
          [granularity = m_c.m_weights.m_granularity](NonOwningReaderSource & source, uint32_t blockSize,
                                                      std::vector<Weight> & values)
      {
        values.resize(blockSize);

        uint32_t prev = ReadVarUint<uint32_t>(source);
        values[0] = granularity * prev;

        for (size_t i = 1; i < blockSize && source.Size() > 0; ++i)
        {
          prev += ReadVarInt<int32_t>(source);
          values[i] = granularity * prev;
        }
      });
    }

    m_c.m_weights.m_loadState = connector::WeightsLoadState::Loaded;
  }

protected:
  bool AddTransition(Transition const & transition, VehicleMask requiredMask)
  {
    if ((transition.GetRoadMask() & requiredMask) == 0)
      return false;

    bool const isOneWay = (transition.GetOneWayMask() & requiredMask) != 0;
    AddTransition(transition.GetCrossMwmId(), transition.GetFeatureId(), transition.GetSegmentIdx(), isOneWay,
                  transition.ForwardIsEnter());
    return true;
  }

  using Weight = connector::Weight;
  using WeightsLoadState = connector::WeightsLoadState;

  // 0 - initial version
  // 1 - removed dummy GeometryCodingParams
  // 2 - store weights as MapUint32ToValue
  static uint32_t constexpr kLastVersion = 2;
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
      : m_size(size)
      , m_numEnters(numEnters)
      , m_numExits(numExits)
      , m_vehicleType(vehicleType)
    {}

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

    Header(uint32_t numTransitions, uint64_t sizeTransitions, uint32_t bitsPerCrossMwmId, uint8_t bitsPerMask)
      : m_numTransitions(numTransitions)
      , m_sizeTransitions(sizeTransitions)
      , m_bitsPerCrossMwmId(bitsPerCrossMwmId)
      , m_bitsPerMask(bitsPerMask)
    {}

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_numTransitions);
      WriteToSink(sink, m_sizeTransitions);
      WriteToSink(sink, m_granularity);
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
      if (m_version > kLastVersion)
      {
        MYTHROW(CorruptedDataException,
                ("Unknown cross mwm section version ", m_version, ", current version ", kLastVersion));
      }

      m_numTransitions = ReadPrimitiveFromSource<decltype(m_numTransitions)>(src);
      m_sizeTransitions = ReadPrimitiveFromSource<decltype(m_sizeTransitions)>(src);
      m_granularity = ReadPrimitiveFromSource<decltype(m_granularity)>(src);
      if (m_version == 0)
        serial::GeometryCodingParams().Load(src);
      m_bitsPerCrossMwmId = ReadPrimitiveFromSource<decltype(m_bitsPerCrossMwmId)>(src);
      m_bitsPerMask = ReadPrimitiveFromSource<decltype(m_bitsPerMask)>(src);

      auto const sectionsSize = ReadPrimitiveFromSource<uint32_t>(src);
      m_sections.resize(base::checked_cast<size_t>(sectionsSize));
      for (Section & section : m_sections)
        section.Deserialize(src);
    }

    void AddSection(Section const & section) { m_sections.push_back(section); }

    uint32_t GetVersion() const { return m_version; }
    uint32_t GetNumTransitions() const { return m_numTransitions; }
    uint64_t GetSizeTransitions() const { return m_sizeTransitions; }
    Weight GetGranularity() const { return m_granularity; }
    uint8_t GetBitsPerCrossMwmId() const { return m_bitsPerCrossMwmId; }
    uint8_t GetBitsPerMask() const { return m_bitsPerMask; }
    std::vector<Section> const & GetSections() const { return m_sections; }

  private:
    uint32_t m_version = kLastVersion;
    uint32_t m_numTransitions = 0;
    uint64_t m_sizeTransitions = 0;
    Weight m_granularity = kGranularity;
    uint32_t m_bitsPerCrossMwmId = 0;
    uint8_t m_bitsPerMask = 0;
    std::vector<Section> m_sections;
  };
};

template <>
inline void CrossMwmConnectorBuilder<base::GeoObjectId>::CheckBitsPerCrossMwmId(uint32_t bitsPerCrossMwmId) const
{
  CHECK_LESS_OR_EQUAL(bitsPerCrossMwmId, connector::kOsmIdBits, ());
}

template <>
inline void CrossMwmConnectorBuilder<connector::TransitId>::CheckBitsPerCrossMwmId(uint32_t bitsPerCrossMwmId) const
{
  CHECK_EQUAL(bitsPerCrossMwmId, connector::CalcBitsPerTransitId(), ());
}

/// Builder class for generator and tests.
template <typename CrossMwmId>
class CrossMwmConnectorBuilderEx : public CrossMwmConnectorBuilder<CrossMwmId>
{
  using BaseT = CrossMwmConnectorBuilder<CrossMwmId>;

private:
  uint32_t CalcBitsPerCrossMwmId() const;

  void WriteTransitions(int32_t bitsPerOsmId, uint8_t bitsPerMask, std::vector<uint8_t> & buffer) const
  {
    MemWriter<std::vector<uint8_t>> memWriter(buffer);

    for (auto const & transition : m_transitions)
      transition.Serialize(bitsPerOsmId, bitsPerMask, memWriter);
  }

  static uint16_t constexpr kBlockSize = 256;

  using Weight = typename BaseT::Weight;
  using IdxWeightT = std::pair<uint32_t, Weight>;

  void WriteWeights(std::vector<uint8_t> & buffer) const
  {
    MapUint32ToValueBuilder<Weight> builder;
    for (auto const & w : m_weights)
      builder.Put(w.first, w.second);

    MemWriter writer(buffer);
    builder.Freeze(writer, [](auto & writer, auto beg, auto end)
    {
      auto const NextStoredValue = [&beg]() { return (*beg++ + BaseT::kGranularity - 1) / BaseT::kGranularity; };

      Weight prev = NextStoredValue();
      WriteVarUint(writer, prev);
      while (beg != end)
      {
        Weight const storedWeight = NextStoredValue();
        WriteVarInt(writer, static_cast<int32_t>(storedWeight) - static_cast<int32_t>(prev));
        prev = storedWeight;
      }
    }, kBlockSize);
  }

public:
  CrossMwmConnectorBuilderEx() : BaseT(m_connector) {}

  void AddTransition(CrossMwmId const & crossMwmId, uint32_t featureId, uint32_t segmentIdx, VehicleMask roadMask,
                     VehicleMask oneWayMask, bool forwardIsEnter)
  {
    m_transitions.emplace_back(crossMwmId, featureId, segmentIdx, roadMask, oneWayMask, forwardIsEnter);
  }

  size_t GetTransitionsCount() const { return m_transitions.size(); }

  template <class Sink>
  void Serialize(Sink & sink)
  {
    uint32_t const bitsPerCrossMwmId = CalcBitsPerCrossMwmId();
    auto const bitsPerMask = uint8_t(VehicleType::Count);
    std::vector<uint8_t> transitionsBuf;
    WriteTransitions(bitsPerCrossMwmId, bitsPerMask, transitionsBuf);

    typename BaseT::Header header(base::checked_cast<uint32_t>(m_transitions.size()),
                                  base::checked_cast<uint64_t>(transitionsBuf.size()), bitsPerCrossMwmId, bitsPerMask);

    std::vector<uint8_t> weightsBuf;
    if (!m_weights.empty())
    {
      std::sort(m_weights.begin(), m_weights.end(), base::LessBy(&IdxWeightT::first));
      WriteWeights(weightsBuf);

      header.AddSection(typename BaseT::Section(weightsBuf.size(), m_connector.GetNumEnters(),
                                                m_connector.GetNumExits(), m_vehicleType));
    }

    // Use buffer serialization above, because BaseT::Header is not plain (vector<Section>)
    // and we are not able to calculate its final size.
    header.Serialize(sink);
    sink.Write(transitionsBuf.data(), transitionsBuf.size());
    sink.Write(weightsBuf.data(), weightsBuf.size());
  }

  typename BaseT::ConnectorT const & PrepareConnector(VehicleType requiredVehicle)
  {
    m_vehicleType = requiredVehicle;
    BaseT::FillTransitions(m_transitions.size(), m_vehicleType, [this](size_t i) { return m_transitions[i]; });
    return m_connector;
  }

  template <class CalcWeight>
  void FillWeights(CalcWeight && calcWeight)
  {
    CHECK(m_vehicleType != VehicleType::Count, ("PrepareConnector should be called"));

    m_weights.reserve(m_connector.GetNumEnters() * m_connector.GetNumExits());
    m_connector.ForEachEnter([&](uint32_t enterIdx, Segment const & enter)
    {
      m_connector.ForEachExit([&](uint32_t exitIdx, Segment const & exit)
      {
        auto const w = calcWeight(enter, exit);
        CHECK_LESS(w, std::numeric_limits<Weight>::max(), ());

        // Edges weights should be >= astar heuristic, so use std::ceil.
        if (w != connector::kNoRoute)
          m_weights.emplace_back(m_connector.GetWeightIndex(enterIdx, exitIdx), static_cast<Weight>(std::ceil(w)));
      });
    });
  }

  /// Used in tests only.
  void SetAndWriteWeights(std::vector<IdxWeightT> && weights, std::vector<uint8_t> & buffer)
  {
    m_weights = std::move(weights);
    std::sort(m_weights.begin(), m_weights.end(), base::LessBy(&IdxWeightT::first));
    WriteWeights(buffer);
  }

private:
  // All accumulated transitions with road-mask inside.
  std::vector<typename BaseT::Transition> m_transitions;

  // Weights for the current prepared connector. Used for VehicleType::Car only now.
  typename BaseT::ConnectorT m_connector;
  std::vector<IdxWeightT> m_weights;
  VehicleType m_vehicleType = VehicleType::Count;
};

template <>
inline uint32_t CrossMwmConnectorBuilderEx<base::GeoObjectId>::CalcBitsPerCrossMwmId() const
{
  uint64_t serial = 0;
  for (auto const & transition : m_transitions)
    serial = std::max(serial, transition.GetCrossMwmId().GetSerialId());

  // Note that we lose base::GeoObjectId::Type bits here, remember about it in ReadCrossMwmId method.
  return bits::NumUsedBits(serial);
}

template <>
inline uint32_t CrossMwmConnectorBuilderEx<connector::TransitId>::CalcBitsPerCrossMwmId() const
{
  return connector::CalcBitsPerTransitId();
}

}  // namespace routing
