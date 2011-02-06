#pragma once

#include "cell_id.hpp"

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
  // needed for hacky coastlines merging
  friend class FeatureBuilder1Merger;

public:
  FeatureBuilder1();

  /// @name Geometry manipulating functions.
  //@{
  /// Set center (origin) point of feature and set that feature is point.
  void SetCenter(m2::PointD const & p);

  /// Add point to geometry.
  void AddPoint(m2::PointD const & p);

  /// Set that feature is linear type.
  void SetLinear() { m_bLinear = true; }

  /// Set that featue is area and get ownership of holes.
  void SetAreaAddHoles(list<vector<m2::PointD> > & holes);
  //@}

  void AddName(string const & name);

  static const int m_maxTypesCount = 7;

  template <class TIter>
  inline void AddTypes(TIter beg, TIter end)
  {
    // !WTF! with GCC
    int const count = min(static_cast<int>(m_maxTypesCount), static_cast<int>(distance(beg, end)));
    m_Types.assign(beg, beg + count);
  }
  inline void SetType(uint32_t type)
  {
    m_Types.clear();
    m_Types.push_back(type);
  }

  void AddLayer(int32_t layer);

  typedef vector<char> buffer_t;

  /// @name Serialization.
  //@{
  void Serialize(buffer_t & data) const;
  void SerializeBase(buffer_t & data, int64_t base) const;

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
    if (m_bPoint)
      toDo(m_Center);
    else
    {
      for (points_t::const_iterator it = m_Geometry.begin(); it != m_Geometry.end(); ++it)
        if (!toDo(*it))
          return;
    }
  }

  m2::PointD CenterPoint() const { return m_Center; }
  //@}

  bool PreSerialize() { return m_bPoint || m_bLinear || m_bArea; }

protected:

  /// @name For diagnostic use only.
  //@{
  bool operator == (FeatureBuilder1 const &) const;

  bool CheckValid() const;
  //@}

  typedef vector<m2::PointD> points_t;

  uint8_t GetHeader() const;

  /// Name. Can be empty. Check HEADER_HAS_NAME.
  string m_Name;

  /// Feature classificator-types. Can not be empty.
  vector<uint32_t> m_Types;

  /// Drawable layer of feature. Can be empty, Check HEADER_HAS_LAYER.
  int32_t m_Layer;

  m2::RectD m_LimitRect;

  /// Can be one of the following:
  /// - point in point-feature
  /// - origin point of text [future] in line-feature
  /// - origin point of text or symbol in area-feature
  m2::PointD m_Center;    // Check  HEADER_HAS_POINT

  /// Can be one of the following:
  /// - geometry in line-feature
  /// - boundary in area-feature
  points_t m_Geometry;    // Check HEADER_IS_LINE

  /// List of holes in area-feature.
  list<points_t> m_Holes; // Check HEADER_IS_AREA

  /// @name This flags can be combined.
  //@{
  bool m_bPoint;    ///< this is point feature (also m_Center exists)
  bool m_bLinear;   ///< this is linear-feature
  bool m_bArea;     ///< this is area-feature
  //@}
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

  bool IsLine() const { return m_bLinear; }
  bool IsArea() const { return m_bArea; }
  bool IsDrawableInRange(int lowS, int highS) const;

  points_t const & GetGeometry() const { return m_Geometry; }
  list<points_t> const & GetHoles() const { return m_Holes; }

  /// @name Overwrite from base_type.
  //@{
  bool PreSerialize(buffers_holder_t const & data);
  void Serialize(buffers_holder_t & data, int64_t base);
  //@}
};

/// Base feature class for storing common data (without geometry).
class FeatureBase
{
  static const int m_maxTypesCount = 7;
public:
  enum FeatureType
  {
    FEATURE_TYPE_POINT = 0,
    FEATURE_TYPE_LINE = 1,
    FEATURE_TYPE_AREA = 2
  };

  FeatureBase() : m_Offset(0) {}

  typedef vector<char> buffer_t;

  inline FeatureType GetFeatureType() const
  {
    uint8_t const h = Header();
    if (h & HEADER_IS_AREA)
      return FEATURE_TYPE_AREA;
    else if (h & HEADER_IS_LINE)
      return FEATURE_TYPE_LINE;
    else
    {
      ASSERT ( h & HEADER_HAS_POINT, () );
      return FEATURE_TYPE_POINT;
    }
  }

  inline uint32_t GetTypesCount() const
  {
    return Header() & m_maxTypesCount;
  }

  inline int32_t GetLayer() const
  {
    if (!(Header() & HEADER_HAS_LAYER))
      return 0;
    if (!m_bCommonParsed)
      ParseCommon();
    return m_Layer;
  }

  inline string GetName() const
  {
    if (!(Header() & HEADER_HAS_NAME))
      return string();
    if (!m_bCommonParsed)
      ParseCommon();
    return m_Name;
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

  enum
  {
    HEADER_HAS_LAYER = 1U << 7,
    HEADER_HAS_NAME = 1U << 6,
    HEADER_IS_AREA = 1U << 5,
    HEADER_IS_LINE = 1U << 4,
    HEADER_HAS_POINT = 1U << 3
  };

  void InitFeatureBuilder(FeatureBuilder1 & fb) const;

  /// @name Statistic functions.
  //@{
  uint32_t GetTypesSize() const { return m_CommonOffset - m_TypesOffset; }
  //@}

protected:
  void Deserialize(buffer_t & data, uint32_t offset, int64_t base);
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
  mutable int32_t m_Layer;
  mutable string m_Name;
  mutable m2::PointD m_Center;

  mutable m2::RectD m_LimitRect;

  int64_t m_base;

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

    int64_t m_base;

    read_source_t(FilesContainerR const & cont)
      : m_cont(cont), m_offset(0), m_base(0)
    {
    }

    void assign(char const * data, uint32_t size)
    {
      m_data.assign(data, data + size);
    }
  };

  FeatureType() {}
  FeatureType(read_source_t & src);

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
      if (Header() & HEADER_HAS_POINT)
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

  mutable bool m_bHeader2Parsed, m_bPointsParsed, m_bTrianglesParsed;

  mutable inner_geom_stat_t m_InnerStats;

  mutable uint32_t m_ptsSimpMask;

  typedef array<uint32_t, 4> offsets_t; // should be synchronized with ARRAY_SIZE(g_arrScales)

  static void ReadOffsets(ArrayByteSource & src, uint8_t mask, offsets_t & offsets);

  static int GetScaleIndex(int scale);
  static int GetScaleIndex(int scale, offsets_t const & offset);

  mutable offsets_t m_ptsOffsets, m_trgOffsets;
};
