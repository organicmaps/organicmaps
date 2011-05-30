#pragma once

#include "cell_id.hpp"
#include "data_header.hpp"
#include "feature_data.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../coding/file_container.hpp"

#include "../base/base.hpp"
#include "../base/buffer_vector.hpp"

#include "../std/string.hpp"
#include "../std/bind.hpp"


class ArrayByteSource;
class FeatureBase;


/// Used for serialization\deserialization of features during --generate_features.
class FeatureBuilder1
{
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

  inline m2::PointD CenterPoint() const { return m_Center; }
  //@}

  bool PreSerialize();

  inline void SetParams(FeatureParams const & params) { m_Params = params; }

protected:

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

/// Base feature class for storing common data (without geometry).
class FeatureBase
{
  static const int m_maxTypesCount = feature::max_types_count;

public:
  FeatureBase() : m_Offset(0) {}

  typedef vector<char> buffer_t;

  feature::EGeomType GetFeatureType() const;

  inline uint8_t GetTypesCount() const
  {
    return ((Header() & feature::HEADER_TYPE_MASK) + 1);
  }

  inline int8_t GetLayer() const
  {
    if (!(Header() & feature::HEADER_HAS_LAYER))
      return 0;

    if (!m_bCommonParsed)
      ParseCommon();

    return m_Params.layer;
  }

  inline bool HasName() const
  {
    return (Header() & feature::HEADER_HAS_NAME) != 0;
  }

  template <class T>
  inline bool ForEachNameRef(T & functor) const
  {
    if (!HasName())
      return false;

    if (!m_bCommonParsed)
      ParseCommon();

    m_Params.name.ForEachRef(functor);
    return true;
  }

  inline m2::RectD GetLimitRect() const
  {
    ASSERT ( m_LimitRect != m2::RectD::GetEmptyRect(), () );
    return m_LimitRect ;
  }

  class GetTypesFn
  {
  public:
    uint32_t m_types[m_maxTypesCount];
    int m_size;

    GetTypesFn() : m_size(0) {}
    void operator() (uint32_t t)
    {
      m_types[m_size++] = t;
    }
  };

  template <typename FunctorT>
  void ForEachTypeRef(FunctorT & f) const
  {
    if (!m_bTypesParsed)
      ParseTypes();

    uint32_t const typeCount = GetTypesCount();
    for (size_t i = 0; i < typeCount; ++i)
      f(m_Types[i]);
  }

  void InitFeatureBuilder(FeatureBuilder1 & fb) const;

  /// @name Statistic functions.
  //@{
  uint32_t GetTypesSize() const { return m_CommonOffset - m_TypesOffset; }
  //@}

protected:
  void Deserialize(buffer_t & data, uint32_t offset, serial::CodingParams const & params);
  string DebugString() const;

protected:
  buffer_t m_Data;
  uint32_t m_Offset;

  friend class FeatureBuilder1;

  void SetHeader(uint8_t h);

  inline char const * DataPtr() const { return &m_Data[m_Offset]; }
  inline uint8_t Header() const { return static_cast<uint8_t>(*DataPtr()); }
  uint32_t CalcOffset(ArrayByteSource const & source) const;

  mutable uint32_t m_Types[m_maxTypesCount];

  mutable FeatureParamsBase m_Params;

  mutable m2::PointD m_Center;

  mutable m2::RectD m_LimitRect;

  serial::CodingParams m_CodingParams;

  static uint32_t const m_TypesOffset = 1;
  mutable uint32_t m_CommonOffset, m_Header2Offset;

  mutable bool m_bTypesParsed, m_bCommonParsed;

  void ParseTypes() const;
  void ParseCommon() const;

  void ParseAll() const;
};

/// Working feature class with geometry.
class FeatureType : public FeatureBase
{
  typedef FeatureBase base_type;

public:
  struct read_source_t
  {
    FilesContainerR m_cont;

    buffer_t m_data;
    uint32_t m_offset;

    feature::DataHeader m_header;

    read_source_t(FilesContainerR const & cont)
      : m_cont(cont), m_offset(0)
    {
    }

    void assign(char const * data, uint32_t size)
    {
      m_data.assign(data, data + size);
    }
  };

  FeatureType() {}
  FeatureType(read_source_t & src);

  /// @param priorities optional array of languages priorities
  /// if NULL, default (0) lang will be used
  string GetPreferredDrawableName(char const * priorities = NULL) const;

  void Deserialize(read_source_t & src);

  /// @name Geometry.
  //@{
  m2::RectD GetLimitRect(int scale) const;

  bool IsEmptyGeometry(int scale) const;

  template <typename FunctorT>
  void ForEachPointRef(FunctorT & f, int scale) const
  {
    if (!m_bPointsParsed)
      ParseGeometry(scale);

    if (m_Points.empty())
    {
      // it's a point feature
      if (GetFeatureType() == feature::GEOM_POINT)
        f(CoordPointT(m_Center.x, m_Center.y));
    }
    else
    {
      for (size_t i = 0; i < m_Points.size(); ++i)
        f(CoordPointT(m_Points[i].x, m_Points[i].y));
    }
  }

  template <typename FunctorT>
  void ForEachPoint(FunctorT f, int scale) const
  {
    ForEachPointRef(f, scale);
  }

  template <typename FunctorT>
  void ForEachTriangleRef(FunctorT & f, int scale) const
  {
    if (!m_bTrianglesParsed)
      ParseTriangles(scale);

    for (size_t i = 0; i < m_Triangles.size();)
    {
      f(m_Triangles[i], m_Triangles[i+1], m_Triangles[i+2]);
      i += 3;
    }
  }

  template <typename FunctorT>
  void ForEachTriangleExRef(FunctorT & f, int scale) const
  {
    f.StartPrimitive(m_Triangles.size());
    ForEachTriangleRef(f, scale);
    f.EndPrimitive();
  }
  //@}

  /// For test cases only.
  string DebugString(int scale) const;

  uint8_t GetRank() const;
  uint32_t GetPopulation() const;

  /// @name Statistic functions.
  //@{
  void ParseBeforeStatistic() const
  {
    if (!m_bHeader2Parsed)
      ParseHeader2();
  }

  struct inner_geom_stat_t
  {
    uint32_t m_Points, m_Strips, m_Size;

    void MakeZero()
    {
      m_Points = m_Strips = m_Size = 0;
    }
  };

  inner_geom_stat_t GetInnerStatistic() const { return m_InnerStats; }

  struct geom_stat_t
  {
    uint32_t m_size, m_count;

    geom_stat_t(uint32_t sz, size_t count)
      : m_size(sz), m_count(static_cast<uint32_t>(count))
    {
    }

    geom_stat_t() : m_size(0), m_count(0) {}
  };

  geom_stat_t GetGeometrySize(int scale) const;
  geom_stat_t GetTrianglesSize(int scale) const;
  //@}

private:
  void ParseHeader2() const;
  uint32_t ParseGeometry(int scale) const;
  uint32_t ParseTriangles(int scale) const;

  void ParseAll(int scale) const;

  // For better result this value should be greater than 17
  // (number of points in inner triangle-strips).
  static const size_t static_buffer = 32;

  typedef buffer_vector<m2::PointD, static_buffer> points_t;
  mutable points_t m_Points, m_Triangles;

  FilesContainerR * m_cont;

  feature::DataHeader const * m_header;

  mutable bool m_bHeader2Parsed, m_bPointsParsed, m_bTrianglesParsed;

  mutable inner_geom_stat_t m_InnerStats;

  mutable uint32_t m_ptsSimpMask;

  typedef array<uint32_t, 4> offsets_t; // should be synchronized with ARRAY_SIZE(g_arrScales)

  void ReadOffsets(ArrayByteSource & src, uint8_t mask, offsets_t & offsets) const;

  /// Get the index for geometry serialization.
  /// @param[in]  scale:
  /// -1 : index for the best geometry
  /// -2 : index for the worst geometry
  /// default : needed geometry
  //@{
  int GetScaleIndex(int scale) const;
  int GetScaleIndex(int scale, offsets_t const & offset) const;
  //@}

  mutable offsets_t m_ptsOffsets, m_trgOffsets;
};
