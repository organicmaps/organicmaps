#pragma once

#include "generator/osm_id.hpp"

#include "indexer/feature.hpp"

#include "coding/file_reader.hpp"
#include "coding/read_write_utils.hpp"

#include "std/bind.hpp"


namespace serial { class CodingParams; }

/// Used for serialization\deserialization of features during --generate_features.
class FeatureBuilder1
{
  /// For debugging
  friend string DebugPrint(FeatureBuilder1 const & f);

public:
  FeatureBuilder1();

  /// @name Geometry manipulating functions.
  //@{
  /// Set center (origin) point of feature and set that feature is point.
  void SetCenter(m2::PointD const & p);

  /// Add point to geometry.
  void AddPoint(m2::PointD const & p);

  /// Set that feature is linear type.
  void SetLinear(bool reverseGeometry = false);

  /// Set that feature is area and get ownership of holes.
  void SetAreaAddHoles(list<vector<m2::PointD> > const & holes);
  inline void SetArea() { m_params.SetGeomType(feature::GEOM_AREA); }

  void AddPolygon(vector<m2::PointD> & poly);
  //@}

  inline feature::Metadata const & GetMetadata() const { return m_params.GetMetadata(); }

  inline feature::EGeomType GetGeomType() const { return m_params.GetGeomType(); }

  inline void AddType(uint32_t type) { m_params.AddType(type); }
  inline bool HasType(uint32_t t) const { return m_params.IsTypeExist(t); }
  inline bool PopExactType(uint32_t type) { return m_params.PopExactType(type); }
  inline void SetType(uint32_t type) { m_params.SetType(type); }

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

  typedef vector<char> buffer_t;

  /// @name Serialization.
  //@{
  void Serialize(buffer_t & data) const;
  void SerializeBase(buffer_t & data, serial::CodingParams const & params, bool needSearializeAdditionalInfo = true) const;

  void Deserialize(buffer_t & data);
  //@}

  /// @name Selectors.
  //@{
  inline m2::RectD GetLimitRect() const { return m_limitRect; }

  bool FormatFullAddress(string & res) const;

  /// Get common parameters of feature.
  FeatureBase GetFeatureBase() const;

  bool IsGeometryClosed() const;
  m2::PointD GetGeometryCenter() const;
  m2::PointD GetKeyPoint() const;

  inline size_t GetPointsCount() const { return GetGeometry().size(); }
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
      for (list<points_t>::const_iterator i = m_polygons.begin(); i != m_polygons.end(); ++i)
      {
        for (points_t::const_iterator j = i->begin(); j != i->end(); ++j)
          if (!toDo(*j))
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

  /// @name For OSM debugging, store original OSM id
  //@{
  void AddOsmId(osm::Id id);
  void SetOsmId(osm::Id id);
  osm::Id GetLastOsmId() const;
  string GetOsmIdsString() const;
  //@}

  bool AddName(string const & lang, string const & name);

  int GetMinFeatureDrawScale() const;

  void SetCoastCell(int64_t iCell, string const & strCell);
  inline bool GetCoastCell(int64_t & cell) const
  {
    if (m_coastCell != -1)
    {
      cell = m_coastCell;
      return true;
    }
    else return false;
  }

  string GetName(int8_t lang = StringUtf8Multilang::DEFAULT_CODE) const;

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

  typedef vector<m2::PointD> points_t;

  inline points_t const & GetGeometry() const { return m_polygons.front(); }

  /// List of geometry polygons.
  list<points_t> m_polygons; // Check HEADER_IS_AREA

  /// Not used in GEOM_POINTs
  int64_t m_coastCell;
};

/// Used for serialization of features during final pass.
class FeatureBuilder2 : public FeatureBuilder1
{
  typedef FeatureBuilder1 base_type;

  typedef vector<uint32_t> offsets_t;

  static void SerializeOffsets(uint32_t mask, offsets_t const & offsets, buffer_t & buffer);

public:

  struct buffers_holder_t
  {
    /// @name input
    //@{
    offsets_t m_ptsOffset, m_trgOffset;
    uint8_t m_ptsMask, m_trgMask;

    uint32_t m_ptsSimpMask;

    points_t m_innerPts, m_innerTrg;
    //@}

    /// @name output
    base_type::buffer_t m_buffer;

    buffers_holder_t() : m_ptsMask(0), m_trgMask(0), m_ptsSimpMask(0) {}
  };

  inline bool IsLine() const { return (GetGeomType() == feature::GEOM_LINE); }
  inline bool IsArea() const { return (GetGeomType() == feature::GEOM_AREA); }
  bool IsDrawableInRange(int lowS, int highS) const;

  inline points_t const & GetOuterPoly() const { return GetGeometry(); }
  inline list<points_t> const & GetPolygons() const { return m_polygons; }

  /// @name Overwrite from base_type.
  //@{
  bool PreSerialize(buffers_holder_t const & data);
  void Serialize(buffers_holder_t & data, serial::CodingParams const & params);
  //@}

  uint64_t GetWayIDForRouting() const;
};

namespace feature
{
  /// Read feature from feature source.
  template <class TSource>
  void ReadFromSourceRowFormat(TSource & src, FeatureBuilder1 & fb)
  {
    uint32_t const sz = ReadVarUint<uint32_t>(src);
    typename FeatureBuilder1::buffer_t buffer(sz);
    src.Read(&buffer[0], sz);
    fb.Deserialize(buffer);
  }

  /// Process features in .dat file.
  template <class ToDo>
  void ForEachFromDatRawFormat(string const & fName, ToDo & toDo)
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
