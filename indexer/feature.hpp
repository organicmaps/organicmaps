#pragma once

#include "cell_id.hpp"

#include "../storage/defines.hpp" // just for file extensions

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../coding/file_container.hpp"

#include "../base/base.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/array.hpp"
#include "../std/bind.hpp"

class ArrayByteSource;
class FeatureBase;

/// Used for serialization\deserialization of features during --generate_features.
class FeatureBuilder1
{
public:
  FeatureBuilder1();

  /// @name Geometry manipulating functions.
  //@{
  /// Set center (origin) point of feature.
  void SetCenter(m2::PointD const & p);

  /// Add point to geometry.
  void AddPoint(m2::PointD const & p);

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
  void SerializeBase(buffer_t & data) const;

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
  //@}

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

  bool m_bArea;       ///< this is area-feature
  bool m_bHasCenter;  ///< m_center exists
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
    offsets_t m_lineOffset;  // in
    offsets_t m_trgOffset;   // in
    uint32_t m_lineMask, m_trgMask;

    base_type::buffer_t m_buffer;   // out

    buffers_holder_t() : m_lineMask(0), m_trgMask(0) {}
  };

  bool IsDrawableLikeLine(int lowS, int highS) const;
  bool IsDrawableLikeArea() const { return m_bArea; }

  points_t const & GetGeometry() const { return m_Geometry; }
  list<points_t> const & GetHoles() const { return m_Holes; }

  /// @name Overwrite from base_type.
  //@{
  void Serialize(buffers_holder_t & data);
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
    if (!m_bLayerParsed)
      ParseLayer();
    return m_Layer;
  }

  inline string GetName() const
  {
    if (!(Header() & HEADER_HAS_NAME))
      return string();
    if (!m_bNameParsed)
      ParseName();
    return m_Name;
  }

  inline m2::RectD GetLimitRect() const
  {
    ASSERT ( m_bGeometryParsed || m_bTrianglesParsed, () );
    return m_LimitRect;
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

protected:
  void Deserialize(buffer_t & data, uint32_t offset = 0);
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

  mutable uint32_t m_LayerOffset;
  mutable uint32_t m_NameOffset;
  mutable uint32_t m_CenterOffset;
  mutable uint32_t m_GeometryOffset;
  mutable uint32_t m_TrianglesOffset;

  mutable bool m_bTypesParsed;
  mutable bool m_bLayerParsed;
  mutable bool m_bNameParsed;
  mutable bool m_bCenterParsed;
  mutable bool m_bGeometryParsed;
  mutable bool m_bTrianglesParsed;

  void ParseTypes() const;
  void ParseLayer() const;
  void ParseName() const;
  void ParseCenter() const;

  void ParseAll() const;
};

/// Working feature class with geometry.
class FeatureType : public FeatureBase
{
  typedef FeatureBase base_type;

public:
  struct read_source_t
  {
    buffer_t m_data;
    uint32_t m_offset;

    FilesContainerR m_cont;

    read_source_t(FilesContainerR const & cont) : m_offset(0), m_cont(cont) {}

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
    if (!m_bGeometryParsed)
      ParseGeometry(scale);

    if (m_Geometry.empty())
    {
      CHECK ( Header() & HEADER_HAS_POINT, ("Call ForEachPoint for empty geometry") );
      f(CoordPointT(m_Center.x, m_Center.y));
    }
    else
    {
      for (size_t i = 0; i < m_Geometry.size(); ++i)
        f(CoordPointT(m_Geometry[i].x, m_Geometry[i].y));
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

private:
  void ParseOffsets() const;
  void ParseGeometry(int scale) const;
  void ParseTriangles(int scale) const;

  void ParseAll(int scale) const;

  mutable vector<m2::PointD> m_Geometry;
  mutable vector<m2::PointD> m_Triangles;

  FilesContainerR * m_cont;

  mutable bool m_bOffsetsParsed;

  typedef array<uint32_t, 4> offsets_t; // should be synhronized with ARRAY_SIZE(g_arrScales)

  static void ReadOffsetsImpl(ArrayByteSource & src, offsets_t & offsets);

  static uint32_t const m_invalidOffset = uint32_t(-1);

  uint32_t GetOffset(int scale, offsets_t const & offset) const;

  mutable offsets_t m_lineOffsets, m_trgOffsets;
};
