#pragma once

#include "indexer/feature_data.hpp"

#include "coding/file_reader.hpp"
#include "coding/read_write_utils.hpp"

#include "base/geo_object_id.hpp"
#include "base/stl_helpers.hpp"

#include <functional>
#include <list>
#include <string>
#include <vector>

namespace serial
{
class GeometryCodingParams;
}  // namespace serial

/// Used for serialization\deserialization of features during --generate_features.
class FeatureBuilder1
{
  /// For debugging
  friend std::string DebugPrint(FeatureBuilder1 const & f);

public:
  using PointSeq = std::vector<m2::PointD>;
  using Geometry = std::list<PointSeq>;

  using Buffer = std::vector<char>;

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
  void SetAreaAddHoles(Geometry const & holes);
  void SetArea() { m_params.SetGeomType(feature::GEOM_AREA); }

  bool IsPoint() const { return (GetGeomType() == feature::GEOM_POINT); }
  bool IsLine() const { return (GetGeomType() == feature::GEOM_LINE); }
  bool IsArea() const { return (GetGeomType() == feature::GEOM_AREA); }

  void AddPolygon(std::vector<m2::PointD> & poly);

  void ResetGeometry();
  //@}


  feature::Metadata const & GetMetadata() const { return m_params.GetMetadata(); }
  feature::Metadata & GetMetadata() { return m_params.GetMetadata(); }
  Geometry const & GetGeometry() const { return m_polygons; }
  PointSeq const & GetOuterGeometry() const { return m_polygons.front(); }
  feature::EGeomType GetGeomType() const { return m_params.GetGeomType(); }

  void AddType(uint32_t type) { m_params.AddType(type); }
  bool HasType(uint32_t t) const { return m_params.IsTypeExist(t); }
  bool PopExactType(uint32_t type) { return m_params.PopExactType(type); }
  void SetType(uint32_t type) { m_params.SetType(type); }
  uint32_t FindType(uint32_t comp, uint8_t level) const { return m_params.FindType(comp, level); }
  FeatureParams::Types const & GetTypes() const { return m_params.m_types; }

  /// Check classificator types for their compatibility with feature geometry type.
  /// Need to call when using any classificator types manipulating.
  /// @return false If no any valid types.
  bool RemoveInvalidTypes();

  /// Clear name if it's not visible in scale range [minS, maxS].
  void RemoveNameIfInvisible(int minS = 0, int maxS = 1000);
  void RemoveUselessNames();

  template <class FnT> bool RemoveTypesIf(FnT fn)
  {
    base::EraseIf(m_params.m_types, fn);
    return m_params.m_types.empty();
  }

  /// @name Serialization.
  //@{
  void Serialize(Buffer & data) const;
  void SerializeBase(Buffer & data, serial::GeometryCodingParams const & params,
                     bool saveAddInfo) const;
  void SerializeBorder(serial::GeometryCodingParams const & params, Buffer & data) const;

  void Deserialize(Buffer & data);
  //@}

  /// @name Selectors.
  //@{
  m2::RectD GetLimitRect() const { return m_limitRect; }

  bool FormatFullAddress(std::string & res) const;

  /// Get common parameters of feature.
  feature::TypesHolder GetTypesHolder() const;

  bool IsGeometryClosed() const;
  m2::PointD GetGeometryCenter() const;
  m2::PointD GetKeyPoint() const;

  size_t GetPointsCount() const;
  size_t GetPolygonsCount() const { return m_polygons.size(); }
  size_t GetTypesCount() const { return m_params.m_types.size(); }
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
      for (PointSeq const & points : m_polygons)
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

  bool PreSerializeAndRemoveUselessNames();

  /// @note This function overrides all previous assigned types.
  /// Set all the parameters, except geometry type (it's set by other functions).
  void SetParams(FeatureParams const & params) { m_params.SetParams(params); }

  FeatureParams const & GetParams() const { return m_params; }
  FeatureParams & GetParams() { return m_params; }

  /// @name For OSM debugging and osm objects replacement, store original OSM id
  //@{
  void AddOsmId(base::GeoObjectId id);
  void SetOsmId(base::GeoObjectId id);
  base::GeoObjectId GetFirstOsmId() const;
  base::GeoObjectId GetLastOsmId() const;
  /// @returns an id of the most general element: node's one if there is no area or relation,
  /// area's one if there is no relation, and relation id otherwise.
  base::GeoObjectId GetMostGenericOsmId() const;
  bool HasOsmId(base::GeoObjectId const & id) const;
  std::vector<base::GeoObjectId> const & GetOsmIds() const { return m_osmIds; }
  //@}

  uint64_t GetWayIDForRouting() const;

  int GetMinFeatureDrawScale() const;
  bool IsDrawableInRange(int lowScale, int highScale) const;

  void SetCoastCell(int64_t iCell) { m_coastCell = iCell; }
  bool IsCoastCell() const { return (m_coastCell != -1); }

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
  std::vector<base::GeoObjectId> m_osmIds;

  FeatureParams m_params;

  m2::RectD m_limitRect;

  /// Can be one of the following:
  /// - point in point-feature
  /// - origin point of text [future] in line-feature
  /// - origin point of text or symbol in area-feature
  m2::PointD m_center;    // Check  HEADER_HAS_POINT

  /// List of geometry polygons.
  Geometry m_polygons; // Check HEADER_IS_AREA

  /// Not used in GEOM_POINTs
  int64_t m_coastCell;
};

/// Used for serialization of features during final pass.
class FeatureBuilder2 : public FeatureBuilder1
{
  using Base = FeatureBuilder1;
  using Offsets = std::vector<uint32_t>;

  static void SerializeOffsets(uint32_t mask, Offsets const & offsets, Buffer & buffer);

  /// For debugging
  friend std::string DebugPrint(FeatureBuilder2 const & f);

public:
  struct SupportingData
  {
    /// @name input
    //@{
    Offsets m_ptsOffset;
    Offsets m_trgOffset;
    uint8_t m_ptsMask;
    uint8_t m_trgMask;

    uint32_t m_ptsSimpMask;

    PointSeq m_innerPts;
    PointSeq m_innerTrg;
    //@}

    /// @name output
    Base::Buffer m_buffer;

    SupportingData() : m_ptsMask(0), m_trgMask(0), m_ptsSimpMask(0) {}
  };

  /// @name Overwrite from base_type.
  //@{
  bool PreSerializeAndRemoveUselessNames(SupportingData const & data);
  void SerializeLocalityObject(serial::GeometryCodingParams const & params,
                               SupportingData & data) const;
  void Serialize(SupportingData & data, serial::GeometryCodingParams const & params) const;
  //@}

  feature::AddressData const & GetAddressData() const { return m_params.GetAddressData(); }
};

namespace feature
{
  /// Read feature from feature source.
  template <class TSource>
  void ReadFromSourceRawFormat(TSource & src, FeatureBuilder1 & fb)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    typename FeatureBuilder1::Buffer buffer(sz);
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
      ReadFromSourceRawFormat(src, fb);
      toDo(fb, currPos);
      currPos = src.Pos();
    }
  }
}
