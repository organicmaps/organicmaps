#pragma once

#include "../coding/multilang_utf8_string.hpp"
#include "../coding/value_opt_string.hpp"
#include "../coding/reader.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"


class FeatureBase;

namespace feature
{
  enum EGeomType
  {
    GEOM_UNDEFINED = -1,
    // Note! do not change this values. Should be equal with FeatureGeoType.
    GEOM_POINT = 0,
    GEOM_LINE = 1,
    GEOM_AREA = 2
  };

  enum EHeaderMask
  {
    HEADER_TYPE_MASK = 7U,
    HEADER_HAS_NAME = 1U << 3,
    HEADER_HAS_LAYER = 1U << 4,
    HEADER_GEOTYPE_MASK = 3U << 5,
    HEADER_HAS_ADDINFO = 1U << 7
  };

  enum EHeaderTypeMask
  {
    HEADER_GEOM_POINT = 0,
    HEADER_GEOM_LINE = 1U << 5,
    HEADER_GEOM_AREA = 1U << 6
  };

  static const int max_types_count = HEADER_TYPE_MASK + 1;

  enum ELayerFlags
  {
    LAYER_LOW = -11,

    LAYER_EMPTY = 0,
    LAYER_TRANSPARENT_TUNNEL = 11,    // draw transparent road (tunnels)

    LAYER_HIGH = 12
  };

  class TypesHolder
  {
    uint32_t m_types[max_types_count];
    size_t m_size;

    EGeomType m_geoType;

  public:
    TypesHolder(EGeomType geoType = GEOM_UNDEFINED) : m_size(0), m_geoType(geoType) {}
    TypesHolder(FeatureBase const & f);

    /// Accumulation function.
    inline void operator() (uint32_t t) { m_types[m_size++] = t; }

    /// @name Selectors.
    //@{
    inline EGeomType GetGeoType() const { return m_geoType; }

    inline bool Empty() const { return (m_size == 0); }
    inline size_t Size() const { return m_size; }
    inline uint32_t operator[] (size_t i) const
    {
      ASSERT_LESS(i, m_size, ());
      return m_types[i];
    }
    inline uint32_t GetBestType() const
    {
      // 0 - is an empty type.
      return (m_size > 0 ? m_types[0] : 0);
    }

    inline bool Has(uint32_t t) const
    {
      uint32_t const * e = m_types + m_size;
      return (find(m_types, e, t) != e);
    }
    //@}

    void Remove(uint32_t t);
    string DebugPrint() const;

    /// Sort types by it's specification (more detailed type goes first).
    void SortBySpec();
  };

  inline string DebugPrint(TypesHolder const & t)
  {
    return t.DebugPrint();
  }
}

/// Feature description struct.
struct FeatureParamsBase
{
  StringUtf8Multilang name;
  StringNumericOptimal house;
  string ref;
  string flats;
  int8_t layer;
  uint8_t rank;

  FeatureParamsBase() : layer(0), rank(0) {}

  void MakeZero();

  bool operator == (FeatureParamsBase const & rhs) const;

  bool CheckValid() const;

  string DebugString() const;

  void AddHouseName(string const & s);
  void AddHouseNumber(string const & s);

  template <class TSink>
  void Write(TSink & sink, uint8_t header, feature::EGeomType type) const
  {
    if (header & feature::HEADER_HAS_NAME)
      name.Write(sink);

    if (header & feature::HEADER_HAS_LAYER)
      WriteToSink(sink, layer);

    if (header & feature::HEADER_HAS_ADDINFO)
    {
      switch (type)
      {
      case feature::GEOM_POINT:
        WriteToSink(sink, rank);
        break;
      case feature::GEOM_LINE:
        utils::WriteString(sink, ref);
        break;
      case feature::GEOM_AREA:
        house.Write(sink);
        break;
      default:
        ASSERT(false, ("Undefined geometry type"));
      }
    }
  }

  template <class TSrc>
  void Read(TSrc & src, uint8_t header, feature::EGeomType type)
  {
    if (header & feature::HEADER_HAS_NAME)
      name.Read(src);

    if (header & feature::HEADER_HAS_LAYER)
      layer = ReadPrimitiveFromSource<int8_t>(src);

    if (header & feature::HEADER_HAS_ADDINFO)
    {
      switch (type)
      {
      case feature::GEOM_POINT:
        rank = ReadPrimitiveFromSource<uint8_t>(src);
        break;
      case feature::GEOM_LINE:
        utils::ReadString(src, ref);
        break;
      case feature::GEOM_AREA:
        house.Read(src);
        break;
      default:
        ASSERT(false, ("Undefined geometry type"));
      }
    }
  }
};

class FeatureParams : public FeatureParamsBase
{
  typedef FeatureParamsBase BaseT;

  bool m_geomTypes[3];

public:
  typedef vector<uint32_t> types_t;
  types_t m_Types;

  FeatureParams()
  {
    m_geomTypes[0] = m_geomTypes[1] = m_geomTypes[2] = false;
  }

  /// Assign parameters except geometry type.
  /// Geometry is independent state and it's set by FeatureType's geometry functions.
  inline void SetParams(FeatureParams const & rhs)
  {
    BaseT::operator=(rhs);
    m_Types = rhs.m_Types;
  }

  inline bool IsValid() const { return !m_Types.empty(); }

  inline void SetGeomType(feature::EGeomType t) { m_geomTypes[t] = true; }
  inline void RemoveGeomType(feature::EGeomType t) { m_geomTypes[t] = false; }

  feature::EGeomType GetGeomType() const;
  uint8_t GetTypeMask() const;

  inline void AddType(uint32_t t) { m_Types.push_back(t); }

  /// @param skipType2  Do not accumulate this type if skipType2 != 0.
  /// '2' means 2-level type in classificator tree (also skip child types).
  void AddTypes(FeatureParams const & rhs, uint32_t skipType2);

  template <class TIter> void AssignTypes(TIter b, TIter e)
  {
    m_Types.assign(b, e);
  }

  void FinishAddingTypes();

  void SetType(uint32_t t);
  bool PopAnyType(uint32_t & t);
  bool PopExactType(uint32_t t);
  bool IsTypeExist(uint32_t t) const;

  bool operator == (FeatureParams const & rhs) const;

  bool CheckValid() const;

  uint8_t GetHeader() const;

  template <class TSink> void Write(TSink & sink) const
  {
    uint8_t const header = GetHeader();

    WriteToSink(sink, header);

    for (size_t i = 0; i < m_Types.size(); ++i)
      WriteVarUint(sink, GetIndexForType(m_Types[i]));

    BaseT::Write(sink, header, GetGeomType());
  }

  template <class TSrc> void Read(TSrc & src)
  {
    using namespace feature;

    uint8_t const header = ReadPrimitiveFromSource<uint8_t>(src);

    uint8_t const type = (header & HEADER_GEOTYPE_MASK);
    if (type & HEADER_GEOM_LINE) SetGeomType(GEOM_LINE);
    if (type & HEADER_GEOM_AREA) SetGeomType(GEOM_AREA);
    if (type == HEADER_GEOM_POINT) SetGeomType(GEOM_POINT);

    size_t const count = (header & HEADER_TYPE_MASK) + 1;
    for (size_t i = 0; i < count; ++i)
      m_Types.push_back(GetTypeForIndex(ReadVarUint<uint32_t>(src)));

    BaseT::Read(src, header, GetGeomType());
  }

private:
  static uint32_t GetIndexForType(uint32_t t);
  static uint32_t GetTypeForIndex(uint32_t i);
};

string DebugPrint(FeatureParams const & p);
