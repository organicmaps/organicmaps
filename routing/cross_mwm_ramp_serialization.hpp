#pragma once

#include "routing/cross_mwm_ramp.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/vehicle_mask.hpp"

#include "indexer/coding_params.hpp"
#include "indexer/geometry_serialization.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/checked_cast.hpp"

#include <stdint.h>
#include <vector>

namespace routing
{
class CrossMwmRampSerializer final
{
public:
  class Transition final
  {
  public:
    Transition() = default;

    Transition(uint32_t featureId, uint32_t segmentIdx, VehicleMask roadMask,
               VehicleMask oneWayMask, bool forwardIsEnter, m2::PointD const & backPoint,
               m2::PointD const & frontPoint)
      : m_featureId(featureId)
      , m_segmentIdx(segmentIdx)
      , m_backPoint(backPoint)
      , m_frontPoint(frontPoint)
      , m_roadMask(roadMask)
      , m_oneWayMask(oneWayMask)
      , m_forwardIsEnter(forwardIsEnter)
    {
    }

    template <class Sink>
    void Serialize(serial::CodingParams const & codingParams, uint8_t bitsPerMask,
                   Sink & sink) const
    {
      WriteToSink(sink, m_featureId);
      WriteToSink(sink, m_segmentIdx);
      serial::SavePoint(sink, m_backPoint, codingParams);
      serial::SavePoint(sink, m_frontPoint, codingParams);

      BitWriter<Sink> writer(sink);
      writer.WriteAtMost32Bits(static_cast<uint32_t>(m_roadMask), bitsPerMask);
      writer.WriteAtMost32Bits(static_cast<uint32_t>(m_oneWayMask), bitsPerMask);
      writer.Write(m_forwardIsEnter ? 0 : 1, 1);
    }

    template <class Source>
    void Deserialize(serial::CodingParams const & codingParams, uint8_t bitsPerMask, Source & src)
    {
      m_featureId = ReadPrimitiveFromSource<decltype(m_featureId)>(src);
      m_segmentIdx = ReadPrimitiveFromSource<decltype(m_segmentIdx)>(src);
      m_backPoint = serial::LoadPoint(src, codingParams);
      m_frontPoint = serial::LoadPoint(src, codingParams);

      BitReader<Source> reader(src);
      m_roadMask = reader.ReadAtMost32Bits(bitsPerMask);
      m_oneWayMask = reader.ReadAtMost32Bits(bitsPerMask);
      m_forwardIsEnter = reader.Read(1) == 0;
    }

    uint32_t GetFeatureId() const { return m_featureId; }
    uint32_t GetSegmentIdx() const { return m_segmentIdx; }
    m2::PointD const & GetBackPoint() const { return m_backPoint; }
    m2::PointD const & GetFrontPoint() const { return m_frontPoint; }
    bool ForwardIsEnter() const { return m_forwardIsEnter; }
    VehicleMask GetRoadMask() const { return m_roadMask; }
    VehicleMask GetOneWayMask() const { return m_oneWayMask; }

  private:
    uint32_t m_featureId = 0;
    uint32_t m_segmentIdx = 0;
    m2::PointD m_backPoint = {0.0, 0.0};
    m2::PointD m_frontPoint = {0.0, 0.0};
    VehicleMask m_roadMask = 0;
    VehicleMask m_oneWayMask = 0;
    bool m_forwardIsEnter = false;
  };

  CrossMwmRampSerializer() = delete;

  template <class Sink>
  static void Serialize(std::vector<Transition> const & transitions,
                        vector<CrossMwmRamp> const & ramps,
                        serial::CodingParams const & codingParams, Sink & sink)
  {
    auto const bitsPerMask = static_cast<uint8_t>(VehicleType::Count);
    vector<uint8_t> transitionsBuf;
    WriteTransitions(transitions, codingParams, bitsPerMask, transitionsBuf);

    Header header(base::checked_cast<uint32_t>(transitions.size()),
                  base::checked_cast<uint64_t>(transitionsBuf.size()), codingParams, bitsPerMask);
    vector<vector<uint8_t>> weightBuffers(ramps.size());

    for (size_t i = 0; i < ramps.size(); ++i)
    {
      CrossMwmRamp const & ramp = ramps[i];
      if (!ramp.HasWeights())
        continue;

      vector<uint8_t> & buffer = weightBuffers[i];
      WriteWeights(ramp.m_weights, buffer);

      auto numEnters = base::checked_cast<uint32_t>(ramp.GetEnters().size());
      auto numExits = base::checked_cast<uint32_t>(ramp.GetExits().size());
      auto const vehicleType = static_cast<VehicleType>(i);
      header.AddSection(Section(buffer.size(), numEnters, numExits, vehicleType));
    }

    header.Serialize(sink);
    FlushBuffer(transitionsBuf, sink);

    for (auto & buffer : weightBuffers)
      FlushBuffer(buffer, sink);
  }

  template <class Source>
  static void DeserializeTransitions(VehicleType requiredVehicle, CrossMwmRamp & ramp, Source & src)
  {
    Header header;
    header.Deserialize(src);

    uint64_t const transitionsEnd = src.Pos() + header.GetSizeTransitions();
    VehicleMask const requiredMask = GetVehicleMask(requiredVehicle);
    auto const numTransitions = base::checked_cast<size_t>(header.GetNumTransitions());

    for (size_t i = 0; i < numTransitions; ++i)
    {
      Transition transition;
      transition.Deserialize(header.GetCodingParams(), header.GetBitsPerMask(), src);
      AddTransition(transition, requiredMask, ramp);
    }

    if (src.Pos() != transitionsEnd)
    {
      MYTHROW(CorruptedDataException,
              ("Wrong position", src.Pos(), "after decoding transitions, expected:", transitionsEnd,
               "size:", header.GetSizeTransitions()));
    }
  }

  template <class Source>
  static void DeserializeWeights(VehicleType requiredVehicle, CrossMwmRamp & ramp, Source & src)
  {
    CHECK(!ramp.WeightsWereLoaded(), ());

    Header header;
    header.Deserialize(src);
    src.Skip(header.GetSizeTransitions());

    for (Section const & section : header.GetSections())
    {
      if (section.GetVehicleType() != requiredVehicle)
      {
        src.Skip(section.GetSize());
        continue;
      }

      size_t const numEnters = ramp.GetEnters().size();
      size_t const numExits = ramp.GetExits().size();

      if (base::checked_cast<size_t>(section.GetNumEnters()) != numEnters)
      {
        MYTHROW(CorruptedDataException,
                ("Mismatch enters number, section:", section.GetNumEnters(), ", ramp:", numEnters));
      }

      if (base::checked_cast<size_t>(section.GetNumExits()) != numExits)
      {
        MYTHROW(CorruptedDataException,
                ("Mismatch exits number, section:", section.GetNumExits(), ", ramp:", numExits));
      }

      size_t const size = numEnters * numExits;
      ramp.m_weights.reserve(size);
      for (size_t i = 0; i < size; ++i)
      {
        auto const weight = ReadPrimitiveFromSource<uint32_t>(src);
        ramp.m_weights.push_back(static_cast<float>(weight));
      }
      break;
    }

    ramp.m_weightsWereLoaded = true;
  }

  static void AddTransition(Transition const & transition, VehicleMask requiredMask,
                            CrossMwmRamp & ramp)
  {
    if ((transition.GetRoadMask() & requiredMask) == 0)
      return;

    bool const isOneWay = (transition.GetOneWayMask() & requiredMask) != 0;
    ramp.AddTransition(transition.GetFeatureId(), transition.GetSegmentIdx(), isOneWay,
                       transition.ForwardIsEnter(), transition.GetBackPoint(),
                       transition.GetFrontPoint());
  }

private:
  static uint32_t constexpr kLastVersion = 0;

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
           serial::CodingParams const & codingParams, uint8_t bitsPerMask)
      : m_numTransitions(numTransitions)
      , m_sizeTransitions(sizeTransitions)
      , m_codingParams(codingParams)
      , m_bitsPerMask(bitsPerMask)
    {
    }

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_numTransitions);
      WriteToSink(sink, m_sizeTransitions);
      m_codingParams.Save(sink);
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
      m_codingParams.Load(src);
      m_bitsPerMask = ReadPrimitiveFromSource<decltype(m_bitsPerMask)>(src);

      auto const sectionsSize = ReadPrimitiveFromSource<uint32_t>(src);
      m_sections.resize(base::checked_cast<size_t>(sectionsSize));
      for (Section & section : m_sections)
        section.Deserialize(src);
    }

    void AddSection(Section const & section) { m_sections.push_back(section); }

    uint32_t GetNumTransitions() const { return m_numTransitions; }
    uint64_t GetSizeTransitions() const { return m_sizeTransitions; }
    serial::CodingParams const & GetCodingParams() const { return m_codingParams; }
    uint8_t GetBitsPerMask() const { return m_bitsPerMask; }
    vector<Section> const & GetSections() const { return m_sections; }

  private:
    uint32_t m_version = kLastVersion;
    uint32_t m_numTransitions = 0;
    uint64_t m_sizeTransitions = 0;
    serial::CodingParams m_codingParams;
    uint8_t m_bitsPerMask = 0;
    vector<Section> m_sections;
  };

  template <class Sink>
  static void FlushBuffer(vector<uint8_t> & buffer, Sink & sink)
  {
    sink.Write(buffer.data(), buffer.size());
    buffer.clear();
  }

  static void WriteTransitions(std::vector<Transition> const & transitions,
                               serial::CodingParams const & codingParams, uint8_t bitsPerMask,
                               std::vector<uint8_t> & buffer);

  static void WriteWeights(std::vector<CrossMwmRamp::Weight> const & weights,
                           std::vector<uint8_t> & buffer);
};
}  // namespace routing
