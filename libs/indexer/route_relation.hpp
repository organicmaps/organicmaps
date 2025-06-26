#pragma once

#include "coding/read_write_utils.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/buffer_vector.hpp"

#include "drape/color.hpp"

#include <array>
#include <string>

namespace feature
{

class RouteRelationBase  // like RelationType (FeatureType)
{
public:
  enum class Type : uint8_t
  {
    Foot = 0,
    Hiking,

    Bicycle,
    MTB,

    Bus,
    Train,
    Tram,
    Trolleybus,
  };
  enum IdxAndFlags : uint8_t
  {
    CycleNetworkIdx = 0,
    OperatorIdx = 1,
    OfficialRefIdx = 2,
    FromIdx = 3,
    ToIdx = 4,
    WikipediaIdx = 5,
    CountIdx = 6,

    HasColor = 0x40,
    HasName = 0x80,
  };

  static constexpr dp::Color kEmptyColor = dp::Color::Transparent();

  void SetParam(std::string_view s, IdxAndFlags idx) { m_params[idx] = s; }
  std::string_view GetParam(IdxAndFlags idx) const { return m_params[idx]; }

  Type GetType() const { return m_type; }
  dp::Color GetColor() const { return m_color; }
  std::string const & GetRef() const { return m_ref; }
  std::string const & GetNetwork() const { return m_network; }

  RouteRelationBase() = default;

  /// @todo Can optimize by storing color as an index in a palette (1 byte).

  template <class TSink>
  void Write(TSink & sink) const
  {
    WriteToSink(sink, uint8_t(m_type));

    uint8_t flags = 0;

    if (m_color != kEmptyColor)
      flags |= HasColor;
    if (!m_name.IsEmpty())
      flags |= HasName;

    for (int i = 0; i < CountIdx; ++i)
      if (!m_params[i].empty())
        flags |= (1 << i);

    WriteToSink(sink, flags);

    if (flags & HasColor)
      WriteToSink(sink, m_color.GetRGBA());

    if (flags & HasName)
      m_name.WriteNonEmpty(sink);

    rw::Write(sink, m_network);
    rw::Write(sink, m_ref);

    for (int i = 0; i < CountIdx; ++i)
      if (!m_params[i].empty())
        rw::WriteNonEmpty(sink, m_params[i]);
  }

  template <class TSource>
  void Read(TSource & src)
  {
    m_type = static_cast<Type>(ReadPrimitiveFromSource<uint8_t>(src));

    uint8_t const flags = ReadPrimitiveFromSource<uint8_t>(src);

    if (flags & HasColor)
      m_color = dp::Color::FromRGBA(ReadPrimitiveFromSource<uint32_t>(src));

    if (flags & HasName)
      m_name.ReadNonEmpty(src);

    rw::Read(src, m_network);
    rw::Read(src, m_ref);

    for (int i = 0; i < CountIdx; ++i)
      if (flags & (1 << i))
        rw::ReadNonEmpty(src, m_params[i]);
  }

protected:
  StringUtf8Multilang m_name;
  std::string m_network, m_ref;
  std::array<std::string, CountIdx> m_params;

  dp::Color m_color = kEmptyColor;
  Type m_type;  // from route or route_master tag

  friend class RelationBuilder;
};

using ShortArray = buffer_vector<uint32_t, 2>;

class RouteRelation : public RouteRelationBase
{
public:
  enum Flags : uint8_t
  {
    HasRelationMembers = 0x1,
    HasParents = 0x2,
  };

  RouteRelation() = default;

  /// @todo Use RecordReader + buffers?
  template <class TSource>
  void Read(TSource & src)
  {
    RouteRelationBase::Read(src);

    uint32_t const sz = ReadVarUint<uint32_t>(src);
    m_ftMembers.reserve(sz);
    uint32_t prev = 0;
    for (size_t i = 0; i < sz; ++i)
    {
      m_ftMembers.push_back(prev + ReadVarUint<uint32_t>(src));
      prev = m_ftMembers.back();
    }

    uint8_t const flags = ReadPrimitiveFromSource<uint8_t>(src);

    if (flags & HasRelationMembers)
      ReadVarUInt32SortedShortArray(src, m_relMembers);

    if (flags & HasParents)
      ReadVarUInt32SortedShortArray(src, m_relParents);
  }

private:
  std::vector<uint32_t> m_ftMembers;
  ShortArray m_relMembers;
  ShortArray m_relParents;
};

}  // namespace feature
