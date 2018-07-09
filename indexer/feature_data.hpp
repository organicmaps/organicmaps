#pragma once
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"

#include "geometry/point2d.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/reader.hpp"
#include "coding/value_opt_string.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/utility.hpp"

struct FeatureParamsBase;
class FeatureBase;

namespace feature
{
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
    /// Coding geometry feature type in 2 bits.
    HEADER_GEOM_POINT = 0,          /// point feature (addinfo = rank)
    HEADER_GEOM_LINE = 1U << 5,     /// linear feature (addinfo = ref)
    HEADER_GEOM_AREA = 1U << 6,     /// area feature (addinfo = house)
    HEADER_GEOM_POINT_EX = 3U << 5  /// point feature (addinfo = house)
  };

  static constexpr int kMaxTypesCount = HEADER_TYPE_MASK + 1;

  enum ELayerFlags
  {
    LAYER_LOW = -11,

    LAYER_EMPTY = 0,
    LAYER_TRANSPARENT_TUNNEL = 11,

    LAYER_HIGH = 12
  };

  class TypesHolder
  {
    uint32_t m_types[kMaxTypesCount];
    size_t m_size;

    EGeomType m_geoType;

  public:
    TypesHolder(EGeomType geoType = GEOM_UNDEFINED) : m_size(0), m_geoType(geoType) {}
    TypesHolder(FeatureBase const & f);

    void Assign(uint32_t type)
    {
      m_types[0] = type;
      m_size = 1;
    }

    /// Accumulation function.
    void Add(uint32_t type)
    {
      ASSERT_LESS(m_size, kMaxTypesCount, ());
      if (m_size < kMaxTypesCount)
        m_types[m_size++] = type;
    }

    /// @name Selectors.
    //@{
    inline EGeomType GetGeoType() const { return m_geoType; }

    inline size_t Size() const { return m_size; }
    inline bool Empty() const { return (m_size == 0); }
    inline uint32_t const * begin() const { return m_types; }
    inline uint32_t const * end() const { return m_types + m_size; }

    /// Assume that m_types is already sorted by SortBySpec function.
    inline uint32_t GetBestType() const
    {
      // 0 - is an empty type.
      return (m_size > 0 ? m_types[0] : 0);
    }

    inline bool Has(uint32_t t) const
    {
      return (find(begin(), end(), t) != end());
    }
    //@}

    template <class TFn> bool RemoveIf(TFn && fn)
    {
      if (m_size > 0)
      {
        size_t const oldSize = m_size;

        uint32_t * e = remove_if(m_types, m_types + m_size, forward<TFn>(fn));
        m_size = distance(m_types, e);

        return (m_size != oldSize);
      }
      return false;
    }

    void Remove(uint32_t type);

    /// Sort types by it's specification (more detailed type goes first).
    void SortBySpec();

    /// Returns true if this->m_types and other.m_types contain same values
    /// in any order. Works in O(n log n).
    bool Equals(TypesHolder const & other) const;

    vector<string> ToObjectNames() const;
  };

  string DebugPrint(TypesHolder const & holder);

  uint8_t CalculateHeader(size_t const typesCount, uint8_t const headerGeomType,
                          FeatureParamsBase const & params);
}  // namespace feature

/// Feature description struct.
struct FeatureParamsBase
{
  StringUtf8Multilang name;
  StringNumericOptimal house;
  string ref;
  int8_t layer;
  uint8_t rank;

  FeatureParamsBase() : layer(0), rank(0) {}

  void MakeZero();

  bool operator == (FeatureParamsBase const & rhs) const;

  bool CheckValid() const;
  string DebugString() const;

  /// @return true if feature doesn't have any drawable strings (names, houses, etc).
  bool IsEmptyNames() const;

  template <class TSink>
  void Write(TSink & sink, uint8_t header) const
  {
    using namespace feature;

    if (header & HEADER_HAS_NAME)
      name.Write(sink);

    if (header & HEADER_HAS_LAYER)
      WriteToSink(sink, layer);

    if (header & HEADER_HAS_ADDINFO)
    {
      switch (header & HEADER_GEOTYPE_MASK)
      {
      case HEADER_GEOM_POINT:
        WriteToSink(sink, rank);
        break;
      case HEADER_GEOM_LINE:
        utils::WriteString(sink, ref);
        break;
      case HEADER_GEOM_AREA:
      case HEADER_GEOM_POINT_EX:
        house.Write(sink);
        break;
      }
    }
  }

  template <class TSrc>
  void Read(TSrc & src, uint8_t header)
  {
    using namespace feature;

    if (header & HEADER_HAS_NAME)
      name.Read(src);

    if (header & HEADER_HAS_LAYER)
      layer = ReadPrimitiveFromSource<int8_t>(src);

    if (header & HEADER_HAS_ADDINFO)
    {
      switch (header & HEADER_GEOTYPE_MASK)
      {
      case HEADER_GEOM_POINT:
        rank = ReadPrimitiveFromSource<uint8_t>(src);
        break;
      case HEADER_GEOM_LINE:
        utils::ReadString(src, ref);
        break;
      case HEADER_GEOM_AREA:
      case HEADER_GEOM_POINT_EX:
        house.Read(src);
        break;
      }
    }
  }
};

class FeatureParams : public FeatureParamsBase
{
  typedef FeatureParamsBase BaseT;

  uint8_t m_geomType;

  feature::Metadata m_metadata;
  feature::AddressData m_addrTags;

public:
  typedef vector<uint32_t> TTypes;
  TTypes m_Types;

  bool m_reverseGeometry;

  FeatureParams() : m_geomType(0xFF), m_reverseGeometry(false) {}

  void ClearName();

  bool AddName(string const & lang, string const & s);
  bool AddHouseName(string const & s);
  bool AddHouseNumber(string houseNumber);

  /// @name Used in storing full street address only.
  //@{
  void AddStreet(string s);
  void AddPlace(string const & s);
  void AddPostcode(string const & s);
  void AddAddress(string const & s);

  bool FormatFullAddress(m2::PointD const & pt, string & res) const;
  //@}

  /// Used for testing purposes now.
  string GetStreet() const;
  feature::AddressData const & GetAddressData() const { return m_addrTags; }

  /// Assign parameters except geometry type.
  /// Geometry is independent state and it's set by FeatureType's geometry functions.
  inline void SetParams(FeatureParams const & rhs)
  {
    BaseT::operator=(rhs);

    m_Types = rhs.m_Types;
    m_addrTags = rhs.m_addrTags;
    m_metadata = rhs.m_metadata;
  }

  inline bool IsValid() const { return !m_Types.empty(); }

  void SetGeomType(feature::EGeomType t);
  void SetGeomTypePointEx();
  feature::EGeomType GetGeomType() const;

  inline void AddType(uint32_t t) { m_Types.push_back(t); }

  /// Special function to replace a regular railway station type with
  /// the special subway type for the correspondent city.
  void SetRwSubwayType(char const * cityName);

  /// @param skipType2  Do not accumulate this type if skipType2 != 0.
  /// '2' means 2-level type in classificator tree (also skip child types).
  void AddTypes(FeatureParams const & rhs, uint32_t skipType2);

  bool FinishAddingTypes();

  void SetType(uint32_t t);
  bool PopAnyType(uint32_t & t);
  bool PopExactType(uint32_t t);
  bool IsTypeExist(uint32_t t) const;

  /// Find type that matches "comp" with "level" in classificator hierarchy.
  uint32_t FindType(uint32_t comp, uint8_t level) const;

  bool CheckValid() const;

  uint8_t GetHeader() const;

  feature::Metadata const & GetMetadata() const { return m_metadata; }
  feature::Metadata & GetMetadata() { return m_metadata; }

  /// @param[in] fullStoring \n
  /// - true when saving in temporary files after first generation step \n
  /// - false when final mwm saving
  template <class TSink> void Write(TSink & sink, bool fullStoring) const
  {
    uint8_t const header = GetHeader();

    WriteToSink(sink, header);

    for (size_t i = 0; i < m_Types.size(); ++i)
      WriteVarUint(sink, GetIndexForType(m_Types[i]));

    if (fullStoring)
    {
      m_metadata.Serialize(sink);
      m_addrTags.Serialize(sink);
    }

    BaseT::Write(sink, header);
  }

  template <class TSource> void Read(TSource & src)
  {
    using namespace feature;

    uint8_t const header = ReadPrimitiveFromSource<uint8_t>(src);
    m_geomType = header & HEADER_GEOTYPE_MASK;

    size_t const count = (header & HEADER_TYPE_MASK) + 1;
    for (size_t i = 0; i < count; ++i)
      m_Types.push_back(GetTypeForIndex(ReadVarUint<uint32_t>(src)));

    m_metadata.Deserialize(src);
    m_addrTags.Deserialize(src);

    BaseT::Read(src, header);
  }

private:
  uint8_t GetTypeMask() const;

  static uint32_t GetIndexForType(uint32_t t);
  static uint32_t GetTypeForIndex(uint32_t i);
};

string DebugPrint(FeatureParams const & p);
