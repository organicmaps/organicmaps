#pragma once
#include "classificator.hpp"

#include "../coding/multilang_utf8_string.hpp"
#include "../coding/value_opt_string.hpp"
#include "../coding/reader.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace feature
{
  enum EGeomType
  {
    GEOM_UNDEFINED = -1,
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
}

/// Feature description struct.
struct FeatureParamsBase
{
  StringUtf8Multilang name;
  StringNumericOptimal house;
  string ref;
  int8_t layer;
  uint8_t rank;

  FeatureParamsBase() : layer(0), rank(0) {}

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
  bool m_geomTypes[3];

public:
  typedef vector<uint32_t> types_t;
  types_t m_Types;

  FeatureParams(FeatureParamsBase const & rhs)
    : FeatureParamsBase(rhs)
  {
    m_geomTypes[0] = m_geomTypes[1] = m_geomTypes[2] = false;
  }
  FeatureParams()
  {
    m_geomTypes[0] = m_geomTypes[1] = m_geomTypes[2] = false;
  }

  bool IsValid() const { return !m_Types.empty(); }

  inline void SetGeomType(feature::EGeomType t) { m_geomTypes[t] = true; }
  inline void RemoveGeomType(feature::EGeomType t) { m_geomTypes[t] = false; }

  feature::EGeomType GetGeomType() const;
  uint8_t GetTypeMask() const;

  inline void AddType(uint32_t t) { m_Types.push_back(t); }
  void AddTypes(FeatureParams const & rhs);
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

    Classificator & c = classif();

    for (size_t i = 0; i < m_Types.size(); ++i)
      WriteVarUint(sink, c.GetIndexForType(m_Types[i]));

    FeatureParamsBase::Write(sink, header, GetGeomType());
  }
};
