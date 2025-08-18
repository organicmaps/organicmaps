#pragma once

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace routing
{
/// \brief Restriction to modify road graph.
struct Restriction
{
  /// \brief Types of road graph restrictions.
  /// \note Despite the fact more that 10 restriction tags are present in osm all of them
  /// could be split into two categories.
  /// * no_left_turn, no_right_turn, no_u_turn and so on go to "No" category.
  /// * only_left_turn, only_right_turn and so on go to "Only" category.
  /// That's enough to remember if
  /// * there's only way to pass the junction is driving along the restriction (Only)
  /// * driving along the restriction is prohibited (No)
  enum class Type
  {
    No,    // Going according such restriction is prohibited.
    Only,  // Only going according such restriction is permitted.

    NoUTurn,   // Making U-turn from a feature to the same one according
               // to such restriction is prohibited.
    OnlyUTurn  // Only U-turn is permitted at the feature.
  };

  Restriction(Type type, std::vector<uint32_t> const & links) : m_featureIds(links), m_type(type) {}
  bool operator==(Restriction const & restriction) const;
  bool operator<(Restriction const & restriction) const;

  // Links of the restriction in feature ids term.
  std::vector<uint32_t> m_featureIds;
  Type m_type;
};

bool IsUTurnType(Restriction::Type type);

// Each std::vector<uint32_t> contains feature ids of a restriction of |Restriction::Type|.
using RestrictionVec = std::vector<std::vector<uint32_t>>;

struct RestrictionUTurn
{
  RestrictionUTurn(uint32_t featureId, bool viaIsFirstPoint)
    : m_featureId(featureId)
    , m_viaIsFirstPoint(viaIsFirstPoint)
  {}

  uint32_t m_featureId;
  bool m_viaIsFirstPoint;
};

struct RestrictionHeader
{
  // In 1.0 version additional data about U-turns was added to the section. 21.05.2019
  static uint16_t constexpr kLatestVersion = 1;

  static std::vector<Restriction::Type> const kRestrictionTypes;

  RestrictionHeader() { Reset(); }

  template <class Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_reserved);

    for (auto const type : kRestrictionTypes)
      CHECK_NOT_EQUAL(m_restrictionCount.count(type), 0, ());

    for (auto const type : kRestrictionTypes)
      WriteToSink(sink, m_restrictionCount.at(type));
  }

  template <class Source>
  void Deserialize(Source & src)
  {
    m_version = ReadPrimitiveFromSource<uint16_t>(src);
    m_reserved = ReadPrimitiveFromSource<uint16_t>(src);
    m_restrictionCount[Restriction::Type::No] = ReadPrimitiveFromSource<uint32_t>(src);
    m_restrictionCount[Restriction::Type::Only] = ReadPrimitiveFromSource<uint32_t>(src);

    // TODO Version 1 was implemented at 21.05.2019.
    //  Supporting version 0 for backward compatibility should be removed later.
    switch (m_version)
    {
    case 0:
    {
      m_restrictionCount[Restriction::Type::NoUTurn] = 0;
      m_restrictionCount[Restriction::Type::OnlyUTurn] = 0;
      break;
    }
    case 1:
    {
      m_restrictionCount[Restriction::Type::NoUTurn] = ReadPrimitiveFromSource<uint32_t>(src);
      m_restrictionCount[Restriction::Type::OnlyUTurn] = ReadPrimitiveFromSource<uint32_t>(src);
      break;
    }
    default: CHECK(false, ("Wrong restrictions version in header:", m_version));
    }
  }

  void Reset();

  uint32_t GetNumberOf(Restriction::Type type) const;
  void SetNumberOf(Restriction::Type type, uint32_t size);

  uint16_t m_version;
  uint16_t m_reserved;
  std::unordered_map<Restriction::Type, uint32_t> m_restrictionCount;
};

class RestrictionSerializer
{
public:
  static uint32_t constexpr kUTurnAtTheBeginMask = 1U << 31;

  template <class Sink>
  static void Serialize(RestrictionHeader const & header, std::vector<Restriction>::iterator begin,
                        std::vector<Restriction>::iterator end, Sink & sink)
  {
    auto const firstOnlyIt = begin + header.GetNumberOf(Restriction::Type::No);
    auto const firstNoUTurnIt = firstOnlyIt + header.GetNumberOf(Restriction::Type::Only);
    auto const firstOnlyUTurnIt = firstNoUTurnIt + header.GetNumberOf(Restriction::Type::NoUTurn);

    SerializeNotUTurn(begin, firstOnlyIt, sink);
    SerializeNotUTurn(firstOnlyIt, firstNoUTurnIt, sink);

    SerializeUTurn(firstNoUTurnIt, firstOnlyUTurnIt, sink);
    SerializeUTurn(firstOnlyUTurnIt, end, sink);
  }

  template <class Source>
  static void Deserialize(RestrictionHeader const & header, RestrictionVec & restrictionsNo,
                          RestrictionVec & restrictionsOnly, std::vector<RestrictionUTurn> & restrictionsNoUTurn,
                          std::vector<RestrictionUTurn> & restrictionsOnlyUTurn, Source & src)
  {
    DeserializeNotUTurn(header, header.GetNumberOf(Restriction::Type::No), restrictionsNo, src);
    DeserializeNotUTurn(header, header.GetNumberOf(Restriction::Type::Only), restrictionsOnly, src);

    DeserializeUTurn(header.GetNumberOf(Restriction::Type::NoUTurn), restrictionsNoUTurn, src);
    DeserializeUTurn(header.GetNumberOf(Restriction::Type::OnlyUTurn), restrictionsOnlyUTurn, src);
  }

private:
  static uint32_t const kDefaultFeatureId = 0;

  /// \brief Serializes a range of restrictions form |begin| to |end| to |sink|.
  /// \param begin is an iterator to the first item to serialize.
  /// \param end is an iterator to the element after the last element to serialize.
  /// \note All restrictions should have the same type.
  template <class Sink>
  static void SerializeNotUTurn(std::vector<Restriction>::const_iterator begin,
                                std::vector<Restriction>::const_iterator end, Sink & sink)
  {
    if (begin == end)
      return;

    CHECK(std::is_sorted(begin, end), ());
    Restriction::Type const type = begin->m_type;

    CHECK(!IsUTurnType(type), ("UTurn restrictions must be processed by another function."));

    uint32_t prevFirstLinkFeatureId = 0;
    BitWriter<FileWriter> bits(sink);
    for (; begin != end; ++begin)
    {
      CHECK_EQUAL(type, begin->m_type, ());

      Restriction const & restriction = *begin;
      CHECK_GREATER_OR_EQUAL(restriction.m_featureIds.size(), 2, ("No sense in zero or one link restrictions."));

      coding::DeltaCoder::Encode(bits, restriction.m_featureIds.size() - 1 /* number of link is two or more */);

      CHECK_LESS_OR_EQUAL(prevFirstLinkFeatureId, restriction.m_featureIds[0], ());
      coding::DeltaCoder::Encode(
          bits, restriction.m_featureIds[0] - prevFirstLinkFeatureId + 1 /* making it greater than zero */);
      for (size_t i = 1; i < restriction.m_featureIds.size(); ++i)
      {
        uint32_t const delta = bits::ZigZagEncode(static_cast<int32_t>(restriction.m_featureIds[i]) -
                                                  static_cast<int32_t>(restriction.m_featureIds[i - 1]));
        coding::DeltaCoder::Encode(bits, delta + 1 /* making it greater than zero */);
      }
      prevFirstLinkFeatureId = restriction.m_featureIds[0];
    }
  }

  template <class Source>
  static bool DeserializeNotUTurn(RestrictionHeader const & header, uint32_t count, RestrictionVec & result,
                                  Source & src)
  {
    uint32_t prevFirstLinkFeatureId = 0;
    BitReader<Source> bits(src);
    for (size_t i = 0; i < count; ++i)
    {
      auto const biasedLinkNumber = coding::DeltaCoder::Decode(bits);
      if (biasedLinkNumber == 0)
      {
        LOG(LERROR, ("Decoded link restriction number is zero."));
        return false;
      }
      auto const numLinks = static_cast<size_t>(biasedLinkNumber + 1 /* number of link is two or more */);

      std::vector<uint32_t> restriction(numLinks);
      auto const biasedFirstFeatureId = coding::DeltaCoder::Decode(bits);
      if (biasedFirstFeatureId == 0)
      {
        LOG(LERROR, ("Decoded first link restriction feature id delta is zero."));
        return false;
      }

      restriction[0] = prevFirstLinkFeatureId + base::checked_cast<uint32_t>(biasedFirstFeatureId) - 1;
      for (size_t j = 1; j < numLinks; ++j)
      {
        auto const biasedDelta = coding::DeltaCoder::Decode(bits);
        if (biasedDelta == 0)
        {
          LOG(LERROR, ("Decoded link restriction feature id delta is zero."));
          return false;
        }

        uint32_t const delta = base::asserted_cast<uint32_t>(biasedDelta - 1);
        restriction[j] = static_cast<uint32_t>(bits::ZigZagDecode(delta) + restriction[j - 1]);
      }

      prevFirstLinkFeatureId = restriction[0];

      // TODO 1.0 version added at 21.05.2019, we should remove supporting 0.0v sometimes.
      //  In 1.0 version there are no two-way restrictions with same featureIds.
      //  Because such restrictions become Restriction::Type::NoUTurn,
      //                                   Restriction::Type::OnlyUTurn.
      if (header.m_version != 0 && restriction.size() == 2)
        CHECK_NOT_EQUAL(restriction.front(), restriction.back(), ());

      result.emplace_back(std::move(restriction));
    }

    return true;
  }

  /// \brief Serializes a range of restrictions form |begin| to |end| to |sink|.
  /// \param begin is an iterator to the first item to serialize.
  /// \param end is an iterator to the element after the last element to serialize.
  /// \note All restrictions should have the same type.
  template <class Sink>
  static void SerializeUTurn(std::vector<Restriction>::iterator begin, std::vector<Restriction>::iterator end,
                             Sink & sink)
  {
    if (begin == end)
      return;

    CHECK(std::is_sorted(begin, end), ());

    Restriction::Type const type = begin->m_type;

    CHECK(IsUTurnType(type), ("Not UTurn restrictions must be processed by another function."));

    auto const savedBegin = begin;
    for (; begin != end; ++begin)
    {
      CHECK_EQUAL(type, begin->m_type, ());
      CHECK_EQUAL(begin->m_featureIds.size(), 1, ("UTurn restrictions must consists of 1 feature."));

      auto featureId = static_cast<int32_t>(begin->m_featureIds.back());
      featureId = bits::ZigZagEncode(featureId);
      begin->m_featureIds.back() = static_cast<uint32_t>(featureId);
    }

    begin = savedBegin;
    std::sort(begin, end);

    uint32_t prevFeatureId = 0;
    for (; begin != end; ++begin)
    {
      Restriction const & restriction = *begin;
      auto const currentFeatureId = restriction.m_featureIds.back();

      CHECK_LESS(prevFeatureId, currentFeatureId, ());
      WriteVarUint(sink, currentFeatureId - prevFeatureId);

      prevFeatureId = currentFeatureId;
    }
  }

  template <class Source>
  static bool DeserializeUTurn(uint32_t count, std::vector<RestrictionUTurn> & result, Source & src)
  {
    uint32_t prevFeatureId = 0;
    BitReader<Source> bits(src);
    for (size_t i = 0; i < count; ++i)
    {
      uint32_t currentFeatureId = 0;
      auto const biasedFirstFeatureId = ReadVarUint<uint32_t>(src);
      if (biasedFirstFeatureId == 0)
      {
        LOG(LERROR, ("Decoded first link restriction feature id delta is zero."));
        return false;
      }

      currentFeatureId = prevFeatureId + biasedFirstFeatureId;
      prevFeatureId = currentFeatureId;

      auto featureIdAfterZigZag = bits::ZigZagDecode(currentFeatureId);

      bool const viaNodeIsFirstFeturePoint = featureIdAfterZigZag & kUTurnAtTheBeginMask;
      featureIdAfterZigZag ^= featureIdAfterZigZag & kUTurnAtTheBeginMask;
      result.emplace_back(featureIdAfterZigZag, viaNodeIsFirstFeturePoint);
    }

    return true;
  }
};

std::string DebugPrint(Restriction::Type const & type);
std::string DebugPrint(Restriction const & restriction);
std::string DebugPrint(RestrictionHeader const & restrictionHeader);
}  // namespace routing
