#pragma once

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include "std/vector.hpp"

namespace routing
{
/// \brief Restriction to modify road graph.
struct Restriction
{
  using FeatureId = uint32_t;
  static FeatureId const kInvalidFeatureId;

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
    No, // Going according such restriction is prohibited.
    Only, // Only going according such restriction is permitted
  };

  Restriction(Type type, size_t linkNumber);
  Restriction(Type type, vector<FeatureId> const & links);

  bool IsValid() const;
  bool operator==(Restriction const & restriction) const;
  bool operator<(Restriction const & restriction) const;

  vector<FeatureId> m_links;
  Type m_type;
};

using RestrictionVec = vector<Restriction>;
}  // namespace routing

namespace feature
{
struct RoutingHeader
{
  RoutingHeader() { Reset(); }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_reserved);
    WriteToSink(sink, m_noRestrictionCount);
    WriteToSink(sink, m_onlyRestrictionCount);
  }

  template <class TSource>
  void Deserialize(TSource & src)
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

static_assert(sizeof(RoutingHeader) == 12, "Wrong header size of routing section.");

class RestrictionSerializer
{
public:
  RestrictionSerializer() : m_restriction(routing::Restriction::Type::No, 0) {}
  explicit RestrictionSerializer(routing::Restriction const & restriction) : m_restriction(restriction) {}

  template <class TSink>
  void Serialize(routing::Restriction const & prevRestriction, TSink & sink) const
  {
    CHECK(m_restriction.IsValid(), ());
    CHECK_EQUAL(m_restriction.m_links.size(), kSupportedLinkNumber,
                ("Only", kSupportedLinkNumber, "links restriction are supported."));
    CHECK_EQUAL(m_restriction.m_links.size(), prevRestriction.m_links.size(), ());

    BitWriter<TSink> bits(sink);
    for (size_t i = 0; i < kSupportedLinkNumber; ++i)
    {
      uint32_t const delta = bits::ZigZagEncode(static_cast<int32_t>(m_restriction.m_links[i]) -
                                                static_cast<int32_t>(prevRestriction.m_links[i]));
      coding::DeltaCoder::Encode(bits, delta + 1 /* making it greater than zero */);
    }
  }

  template <class TSource>
  bool Deserialize(routing::Restriction const & prevRestriction, TSource & src)
  {
    BitReader<TSource> bits(src);
    m_restriction.m_links.resize(kSupportedLinkNumber);
    for (size_t i = 0; i < kSupportedLinkNumber; ++i)
    {
      uint32_t const biasedDelta = coding::DeltaCoder::Decode(bits);
      if (biasedDelta == 0)
      {
        LOG(LERROR, ("Decoded link restriction feature id delta is zero."));
        m_restriction.m_links.clear();
        return false;
      }
      uint32_t const delta = biasedDelta - 1;
      m_restriction.m_links[i] =
          static_cast<routing::Restriction::FeatureId>(bits::ZigZagDecode(delta) +
                                                       prevRestriction.m_links[i]);
    }
    return true;
  }

  routing::Restriction const & GetRestriction() const { return m_restriction; }

  static size_t const kSupportedLinkNumber;

private:
  routing::Restriction m_restriction;
};
}  // namespace feature
