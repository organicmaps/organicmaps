#pragma once

#include "indexer/feature.hpp"

#include "coding/file_reader.hpp"
#include "coding/read_write_utils.hpp"

#include "base/osm_id.hpp"

#include <functional>
#include <list>


namespace serial { class CodingParams; }

/// Used for serialization\deserialization of features during --generate_features.
class FeatureBuilder1
{
  /// For debugging
  friend std::string DebugPrint(FeatureBuilder1 const & f);

public:
  using TPointSeq = vector<m2::PointD>;
  using TGeometry = std::list<TPointSeq>;

  using TBuffer = vector<char>;

  FeatureBuilder1();

  /// @name Geometry manipulating functions.
  //@{
  /// Set center (origin) point of feature and set that feature is point.
  void SetCenter(m2::PointD const & p);

  void SetRank(uint8_t rank);

  void AddHouseNumber(std::string const & houseNumber);

  void AddStreet(std::string const & streetName);

  void AddPostcode(std::string const & postcode);

  /// Add point to geometry.
  void AddPoint(m2::PointD const & p);

  /// Set that feature is linear type.
  void SetLinear(bool reverseGeometry = false);

  /// Set that feature is area and get ownership of holes.
  void SetAreaAddHoles(TGeometry const & holes);
  inline void SetArea() { m_params.SetGeomType(feature::GEOM_AREA); }

  inline bool IsLine() const { return (GetGeomType() == feature::GEOM_LINE); }
  inline bool IsArea() const { return (GetGeomType() == feature::GEOM_AREA); }

  void AddPolygon(vector<m2::PointD> & poly);

  void ResetGeometry();
  //@}


  inline feature::Metadata const & GetMetadata() const { return m_params.GetMetadata(); }
  inline feature::Metadata & GetMetadataForTesting() { return m_params.GetMetadata(); }
  inline TGeometry const & GetGeometry() const { return m_polygons; }
  inline TPointSeq const & GetOuterGeometry() const { return m_polygons.front(); }
  inline feature::EGeomType GetGeomType() const { return m_params.GetGeomType(); }

  inline void AddType(uint32_t type) { m_params.AddType(type); }
  inline bool HasType(uint32_t t) const { return m_params.IsTypeExist(t); }
  inline bool PopExactType(uint32_t type) { return m_params.PopExactType(type); }
  inline void SetType(uint32_t type) { m_params.SetType(type); }
  inline uint32_t FindType(uint32_t comp, uint8_t level) const { return m_params.FindType(comp, level); }
  inline FeatureParams::TTypes const & GetTypes() const  { return m_params.m_Types; }

  /// Check classificator types for their compatibility with feature geometry type.
  /// Need to call when using any classificator types manipulating.
  /// @return false If no any valid types.
  bool RemoveInvalidTypes();

  /// Clear name if it's not visible in scale range [minS, maxS].
  void RemoveNameIfInvisible(int minS = 0, int maxS = 1000);
  void RemoveUselessNames();

  template <class FnT> bool RemoveTypesIf(FnT fn)
  {
    m_params.m_Types.erase(remove_if(m_params.m_Types.begin(), m_params.m_Types.end(), fn),
                           m_params.m_Types.end());
    return m_params.m_Types.empty();
  }

  /// @name Serialization.
  //@{
  void Serialize(TBuffer & data) const;
  void SerializeBase(TBuffer & data, serial::CodingParams const & params, bool saveAddInfo) const;

  void Deserialize(TBuffer & data);
  //@}

  /// @name Selectors.
  //@{
  inline m2::RectD GetLimitRect() const { return m_limitRect; }

  bool FormatFullAddress(std::string & res) const;

  /// Get common parameters of feature.
  FeatureBase GetFeatureBase() const;

  bool IsGeometryClosed() const;
  m2::PointD GetGeometryCenter() const;
  m2::PointD GetKeyPoint() const;

  size_t GetPointsCount() const;
  inline size_t GetPolygonsCount() const { return m_polygons.size(); }
  inline size_t GetTypesCount() const { return m_params.m_Types.size(); }
  //@}

  /// @name Iterate through polygons points.
  /// Stops processing when functor returns false.
  //@{
private:
  template <class ToDo> class ToDoWrapper
  {
    ToDo & m_toDo;
  public:
    ToDoWrapper(ToDo & toDo) : m_toDo(toDo) {}
    bool operator() (m2::PointD const & p) { return m_toDo(p); }
    void EndRegion() {}
  };

public:
  template <class ToDo>
  void ForEachGeometryPointEx(ToDo & toDo) const
  {
    if (m_params.GetGeomType() == feature::GEOM_POINT)
      toDo(m_center);
    else
    {
      for (TPointSeq const & points : m_polygons)
      {
        for (auto const & pt : points)
          if (!toDo(pt))
            return;
        toDo.EndRegion();
      }
    }
  }

  template <class ToDo>
  void ForEachGeometryPoint(ToDo & toDo) const
  {
    ToDoWrapper<ToDo> wrapper(toDo);
    ForEachGeometryPointEx(wrapper);
  }
  //@}

  bool PreSerialize();

  /// @note This function overrides all previous assigned types.
  /// Set all the parameters, except geometry type (it's set by other functions).
  inline void SetParams(FeatureParams const & params) { m_params.SetParams(params); }

  inline FeatureParams const & GetParams() const { return m_params; }

  /// @name For OSM debugging and osm objects replacement, store original OSM id
  //@{
  void AddOsmId(osm::Id id);
  void SetOsmId(osm::Id id);
  osm::Id GetFirstOsmId() const;
  osm::Id GetLastOsmId() const;
  /// @returns an id of the most general element: node's one if there is no area or relation,
  /// area's one if there is no relation, and relation id otherwise.
  osm::Id GetMostGenericOsmId() const;
  bool HasOsmId(osm::Id const & id) const;
  std::string GetOsmIdsString() const;
  vector<osm::Id> const & GetOsmIds() const { return m_osmIds; }
  //@}

  uint64_t GetWayIDForRouting() const;

  int GetMinFeatureDrawScale() const;
  bool IsDrawableInRange(int lowScale, int highScale) const;

  void SetCoastCell(int64_t iCell) { m_coastCell = iCell; }
  inline bool IsCoastCell() const { return (m_coastCell != -1); }

  bool AddName(std::string const & lang, std::string const & name);
  std::string GetName(int8_t lang = StringUtf8Multilang::kDefaultCode) const;

  uint8_t GetRank() const { return m_params.rank; }

  /// @name For diagnostic use only.
  //@{
  bool operator== (FeatureBuilder1 const &) const;

  bool CheckValid() const;
  //@}

  bool IsRoad() const;

protected:
  /// Used for features debugging
  vector<osm::Id> m_osmIds;

  FeatureParams m_params;

  m2::RectD m_limitRect;

  /// Can be one of the following:
  /// - point in point-feature
  /// - origin point of text [future] in line-feature
  /// - origin point of text or symbol in area-feature
  m2::PointD m_center;    // Check  HEADER_HAS_POINT

  /// List of geometry polygons.
  TGeometry m_polygons; // Check HEADER_IS_AREA

  /// Not used in GEOM_POINTs
  int64_t m_coastCell;
};

/// Used for serialization of features during final pass.
class FeatureBuilder2 : public FeatureBuilder1
{
  using TBase = FeatureBuilder1;
  using TOffsets = vector<uint32_t>;

  static void SerializeOffsets(uint32_t mask, TOffsets const & offsets, TBuffer & buffer);

  /// For debugging
  friend std::string DebugPrint(FeatureBuilder2 const & f);

public:
  struct SupportingData
  {
    /// @name input
    //@{
    TOffsets m_ptsOffset;
    TOffsets m_trgOffset;
    uint8_t m_ptsMask;
    uint8_t m_trgMask;

    uint32_t m_ptsSimpMask;

    TPointSeq m_innerPts;
    TPointSeq m_innerTrg;
    //@}

    /// @name output
    TBase::TBuffer m_buffer;

    SupportingData() : m_ptsMask(0), m_trgMask(0), m_ptsSimpMask(0) {}
  };

  /// @name Overwrite from base_type.
  //@{
  bool PreSerialize(SupportingData const & data);
  void Serialize(SupportingData & data, serial::CodingParams const & params);
  //@}

  feature::AddressData const & GetAddressData() const { return m_params.GetAddressData(); }
};

namespace feature
{
  /// Read feature from feature source.
  template <class TSource>
  void ReadFromSourceRowFormat(TSource & src, FeatureBuilder1 & fb)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    typename FeatureBuilder1::TBuffer buffer(sz);
    src.Read(&buffer[0], sz);
    fb.Deserialize(buffer);
  }

  /// Process features in .dat file.
  template <class ToDo>
  void ForEachFromDatRawFormat(std::string const & fName, ToDo && toDo)
  {
    FileReader reader(fName);
    ReaderSource<FileReader> src(reader);

    uint64_t currPos = 0;
    uint64_t const fSize = reader.Size();

    // read features one by one
    while (currPos < fSize)
    {
      FeatureBuilder1 fb;
      ReadFromSourceRowFormat(src, fb);
      toDo(fb, currPos);
      currPos = src.Pos();
    }
  }
}
