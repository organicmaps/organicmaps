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

#include "std/algorithm.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{
/// \brief Restriction to modify road graph.
struct Restriction
{
  static uint32_t const kInvalidFeatureId;

  /// \brief Types of road graph restrictions.
  /// \note Despite the fact more that 10 restriction tags are present in osm all of them
  /// could be split into two categories.
  /// * no_left_turn, no_right_turn, no_u_turn and so on go to "No" category.
  /// * only_left_turn, only_right_turn and so on go to "Only" category.
  /// That's enough to rememeber if
  /// * there's only way to pass the junction is driving along the restriction (Only)
  /// * driving along the restriction is prohibited (No)
  enum class Type
  {
    No,    // Going according such restriction is prohibited.
    Only,  // Only going according such restriction is permitted
  };

  Restriction(Type type, vector<uint32_t> const & links) : m_featureIds(links), m_type(type) {}
  bool IsValid() const;
  bool operator==(Restriction const & restriction) const;
  bool operator<(Restriction const & restriction) const;

  // Links of the restriction in feature ids term.
  vector<uint32_t> m_featureIds;
  Type m_type;
};

using RestrictionVec = vector<Restriction>;

string ToString(Restriction::Type const & type);
string DebugPrint(Restriction::Type const & type);
string DebugPrint(Restriction const & restriction);

struct RestrictionHeader
{
  RestrictionHeader() { Reset(); }

  template <class Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_reserved);
    WriteToSink(sink, m_noRestrictionCount);
    WriteToSink(sink, m_onlyRestrictionCount);
  }

  template <class Source>
  void Deserialize(Source & src)
  {
    m_version = ReadPrimitiveFromSource<uint16_t>(src);
    m_reserved = ReadPrimitiveFromSource<uint16_t>(src);
    m_noRestrictionCount = ReadPrimitiveFromSource<uint32_t>(src);
    m_onlyRestrictionCount = ReadPrimitiveFromSource<uint32_t>(src);
  }

  void Reset()
  {
    m_version = 0;
    m_reserved = 0;
    m_noRestrictionCount = 0;
    m_onlyRestrictionCount = 0;
  }

  uint16_t m_version;
  uint16_t m_reserved;
  uint32_t m_noRestrictionCount;
  uint32_t m_onlyRestrictionCount;
};

static_assert(sizeof(RestrictionHeader) == 12, "Wrong header size of routing section.");

class RestrictionSerializer
{
public:
  template <class Sink>
  static void Serialize(RestrictionHeader const & header,
                        routing::RestrictionVec::const_iterator begin,
                        routing::RestrictionVec::const_iterator end, Sink & sink)
  {
    auto const firstOnlyIt = begin + header.m_noRestrictionCount;
    SerializeSingleType(begin, firstOnlyIt, sink);
    SerializeSingleType(firstOnlyIt, end, sink);
  }

  template <class Source>
  static void Deserialize(RestrictionHeader const & header,
                          routing::RestrictionVec & restrictionsNo,
                          routing::RestrictionVec & restrictionsOnly, Source & src)
  {
    DeserializeSingleType(routing::Restriction::Type::No, header.m_noRestrictionCount,
                          restrictionsNo, src);
    DeserializeSingleType(routing::Restriction::Type::Only, header.m_onlyRestrictionCount,
                          restrictionsOnly, src);
  }

private:
  static uint32_t const kDefaultFeatureId = 0;

  /// \brief Serializes a range of restrictions form |begin| to |end| to |sink|.
  /// \param begin is an iterator to the first item to serialize.
  /// \param end is an iterator to the element after the last element to serialize.
  /// \note All restrictions should have the same type.
  template <class Sink>
  static void SerializeSingleType(routing::RestrictionVec::const_iterator begin,
                                  routing::RestrictionVec::const_iterator end, Sink & sink)
  {
    if (begin == end)
      return;

    CHECK(is_sorted(begin, end), ());
    routing::Restriction::Type const type = begin->m_type;

    uint32_t prevFirstLinkFeatureId = 0;
    BitWriter<FileWriter> bits(sink);
    for (; begin != end; ++begin)
    {
      CHECK_EQUAL(type, begin->m_type, ());

      routing::Restriction const & restriction = *begin;
      CHECK(restriction.IsValid(), ());
      CHECK_LESS(1, restriction.m_featureIds.size(),
                 ("No sense in zero or one link restrictions."));

      coding::DeltaCoder::Encode(
          bits, restriction.m_featureIds.size() - 1 /* number of link is two or more */);

      CHECK_LESS_OR_EQUAL(prevFirstLinkFeatureId, restriction.m_featureIds[0], ());
      coding::DeltaCoder::Encode(bits, restriction.m_featureIds[0] - prevFirstLinkFeatureId +
                                           1 /* making it greater than zero */);
      for (size_t i = 1; i < restriction.m_featureIds.size(); ++i)
      {
        uint32_t const delta =
            bits::ZigZagEncode(static_cast<int32_t>(restriction.m_featureIds[i]) -
                               static_cast<int32_t>(restriction.m_featureIds[i - 1]));
        coding::DeltaCoder::Encode(bits, delta + 1 /* making it greater than zero */);
      }
      prevFirstLinkFeatureId = restriction.m_featureIds[0];
    }
  }

  template <class Source>
  static bool DeserializeSingleType(routing::Restriction::Type type, uint32_t count,
                                    routing::RestrictionVec & restrictions, Source & src)
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
      size_t const numLinks = biasedLinkNumber + 1 /* number of link is two or more */;

      routing::Restriction restriction(type, {} /* links */);
      restriction.m_featureIds.resize(numLinks);
      auto const biasedFirstFeatureId = coding::DeltaCoder::Decode(bits);
      if (biasedFirstFeatureId == 0)
      {
        LOG(LERROR, ("Decoded first link restriction feature id delta is zero."));
        return false;
      }
      restriction.m_featureIds[0] =
          prevFirstLinkFeatureId + base::checked_cast<uint32_t>(biasedFirstFeatureId) - 1;
      for (size_t i = 1; i < numLinks; ++i)
      {
        auto const biasedDelta = coding::DeltaCoder::Decode(bits);
        if (biasedDelta == 0)
        {
          LOG(LERROR, ("Decoded link restriction feature id delta is zero."));
          return false;
        }

        uint32_t const delta = base::asserted_cast<uint32_t>(biasedDelta - 1);
        restriction.m_featureIds[i] =
            static_cast<uint32_t>(bits::ZigZagDecode(delta) + restriction.m_featureIds[i - 1]);
      }

      prevFirstLinkFeatureId = restriction.m_featureIds[0];
      restrictions.push_back(restriction);
    }
    return true;
  }
};
}  // namespace routing
