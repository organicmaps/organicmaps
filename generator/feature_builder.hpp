#pragma once

#include "osm_id.hpp"

#include "../indexer/feature.hpp"

#include "../coding/file_reader.hpp"

#include "../std/bind.hpp"

namespace serial { class CodingParams; }

/// Used for serialization\deserialization of features during --generate_features.
class FeatureBuilder1
{
  /// For debugging
  friend string debug_print(FeatureBuilder1 const & f);

public:
  /// @name Geometry manipulating functions.
  //@{
  /// Set center (origin) point of feature and set that feature is point.
  void SetCenter(m2::PointD const & p);

  /// Add point to geometry.
  void AddPoint(m2::PointD const & p);

  /// Set that feature is linear type.
  void SetLinear() { m_Params.SetGeomType(feature::GEOM_LINE); }

  /// Set that feature is area and get ownership of holes.
  void SetAreaAddHoles(list<vector<m2::PointD> > const & holes);
  //@}

  inline feature::EGeomType GetGeomType() const { return m_Params.GetGeomType(); }

  inline void AddType(uint32_t type) { m_Params.AddType(type); }
  inline bool HasType(uint32_t t) const { return m_Params.IsTypeExist(t); }

  typedef vector<char> buffer_t;

  /// @name Serialization.
  //@{
  void Serialize(buffer_t & data) const;
  void SerializeBase(buffer_t & data, serial::CodingParams const & params) const;

  void Deserialize(buffer_t & data);
  //@}

  ///@name Selectors.
  //@{
  inline m2::RectD GetLimitRect() const { return m_LimitRect; }

  /// Get common parameters of feature.
  FeatureBase GetFeatureBase() const;

  bool IsGeometryClosed() const;

  inline size_t GetPointsCount() const { return m_Geometry.size(); }

  template <class ToDo>
  void ForEachPointRef(ToDo & toDo) const
  {
    for_each(m_Geometry.begin(), m_Geometry.end(), bind<void>(ref(toDo), _1));
  }

  // stops processing when functor returns false
  template <class ToDo>
  void ForEachTruePointRef(ToDo & toDo) const
  {
    if (m_Params.GetGeomType() == feature::GEOM_POINT)
      toDo(m_Center);
    else
    {
      for (points_t::const_iterator it = m_Geometry.begin(); it != m_Geometry.end(); ++it)
        if (!toDo(*it))
          return;
    }
  }
  //@}

  bool PreSerialize();

  inline void SetParams(FeatureParams const & params) { m_Params = params; }

  /// For OSM debugging, store original OSM id
  void AddOsmId(string const & type, uint64_t osmId);

protected:
  /// Used for feature debugging
  vector<osm::OsmId> m_osmIds;

  /// @name For diagnostic use only.
  //@{
  bool operator == (FeatureBuilder1 const &) const;

  bool CheckValid() const;
  //@}

  FeatureParams m_Params;

  m2::RectD m_LimitRect;

  /// Can be one of the following:
  /// - point in point-feature
  /// - origin point of text [future] in line-feature
  /// - origin point of text or symbol in area-feature
  m2::PointD m_Center;    // Check  HEADER_HAS_POINT

  typedef vector<m2::PointD> points_t;

  /// Can be one of the following:
  /// - geometry in line-feature
  /// - boundary in area-feature
  points_t m_Geometry;    // Check HEADER_IS_LINE

  /// List of holes in area-feature.
  list<points_t> m_Holes; // Check HEADER_IS_AREA
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

  bool IsLine() const { return (m_Params.GetTypeMask() & feature::HEADER_GEOM_LINE) != 0; }
  bool IsArea() const { return (m_Params.GetTypeMask() & feature::HEADER_GEOM_AREA) != 0; }
  bool IsDrawableInRange(int lowS, int highS) const;

  points_t const & GetGeometry() const { return m_Geometry; }
  list<points_t> const & GetHoles() const { return m_Holes; }

  /// @name Overwrite from base_type.
  //@{
  bool PreSerialize(buffers_holder_t const & data);
  void Serialize(buffers_holder_t & data, serial::CodingParams const & params);
  //@}
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
