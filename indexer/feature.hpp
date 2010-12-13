#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../base/base.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

class ArrayByteSource;
class FeatureBase;

class FeatureBuilder
{
public:
  FeatureBuilder();

  void AddName(string const & name);
  void AddPoint(m2::PointD const & p);
  void AddTriangle(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c);

  template <class TIter>
  inline void AddTypes(TIter beg, TIter end)
  {
    // 15 - is the maximum count of types (@see Feature::GetTypesCount())
    size_t const count = min(15, static_cast<int>(distance(beg, end)));
    m_Types.assign(beg, beg + count);
  }
  inline void SetType(uint32_t type)
  {
    m_Types.clear();
    m_Types.push_back(type);
  }

  void AddLayer(int32_t layer);

  void Serialize(vector<char> & data) const;

  inline m2::RectD GetLimitRect() const { return m_LimitRect; }

  // Get common parameters of feature.
  FeatureBase GetFeatureBase() const;

  bool IsGeometryClosed() const;
  size_t GetPointsCount() const { return m_Geometry.size(); }

  bool operator == (FeatureBuilder const &) const;

private:
  uint8_t GetHeader() const;

  bool CheckCorrect(vector<char> const & data) const;

  int32_t m_Layer;
  string m_Name;
  vector<uint32_t> m_Types;

  m2::RectD m_LimitRect;

  vector<m2::PointD> m_Geometry;  // store points as is for further processing

  vector<int64_t> m_Triangles;
};

class FeatureBase
{
public:
  enum FeatureType
  {
    FEATURE_TYPE_POINT = 0,
    FEATURE_TYPE_LINE = 1,
    FEATURE_TYPE_AREA = 2,
    FEATURE_TYPE_UNKNOWN = 17
  };

  FeatureBase() : m_Offset(0) {}

  /// @name Use like polymorfic functions. Need to overwrite in derived classes.
  //@{
  void Deserialize(vector<char> & data, uint32_t offset = 0);
  string DebugString() const;
  void InitFeatureBuilder(FeatureBuilder & fb) const;
  //@}

  inline FeatureType GetFeatureType() const
  {
    ASSERT_NOT_EQUAL((Header() >> 4) & 3, 3, (DebugString()));
    return static_cast<FeatureType>((Header() >> 4) & 3);
  }

  inline uint32_t GetTypesCount() const
  {
    return Header() & 0xF;
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
    ASSERT(m_bGeometryParsed, ());
    return m_LimitRect;
  }

  class GetTypesFn
  {
  public:
    vector<uint32_t> m_types;

    GetTypesFn() { m_types.reserve(16); }
    void operator() (uint32_t t)
    {
      m_types.push_back(t);
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
    HEADER_IS_LINE = 1U << 4
  };

protected:
  vector<char> m_Data;
  uint32_t m_Offset;

  friend class FeatureBuilder;

  void SetHeader(uint8_t h);

  inline char const * DataPtr() const { return &m_Data[m_Offset]; }
  inline uint8_t Header() const { return static_cast<uint8_t>(*DataPtr()); }
  uint32_t CalcOffset(ArrayByteSource const & source) const;

  mutable uint32_t m_Types[16];
  mutable int32_t m_Layer;
  mutable string m_Name;

  mutable m2::RectD m_LimitRect;

  mutable uint32_t m_LayerOffset;
  mutable uint32_t m_NameOffset;
  mutable uint32_t m_GeometryOffset;
  mutable uint32_t m_TrianglesOffset;

  mutable bool m_bTypesParsed;
  mutable bool m_bLayerParsed;
  mutable bool m_bNameParsed;
  mutable bool m_bGeometryParsed;
  mutable bool m_bTrianglesParsed;

  void ParseTypes() const;
  void ParseLayer() const;
  void ParseName() const;
};

class FeatureGeom : public FeatureBase
{
  typedef FeatureBase base_type;

public:
  FeatureGeom() {}
  FeatureGeom(vector<char> & data, uint32_t offset = 0);

  /// @name Overwrite from base_type.
  //@{
  void Deserialize(vector<char> & data, uint32_t offset = 0);
  string DebugString() const;
  void InitFeatureBuilder(FeatureBuilder & fb) const;
  //@}

  inline m2::RectD GetLimitRect() const
  {
    if (!m_bGeometryParsed)
      ParseGeometry();
    return base_type::GetLimitRect();
  }

  inline uint32_t GetGeometrySize() const
  {
    if (!m_bGeometryParsed)
      ParseGeometry();
    return m_Geometry.size();
  }

  inline uint32_t GetTriangleCount() const
  {
    if (!m_bTrianglesParsed)
      ParseTriangles();
    return (m_Triangles.size() / 3);
  }

  template <typename FunctorT>
  void ForEachPointRef(FunctorT & f) const
  {
    if (!m_bGeometryParsed)
      ParseGeometry();
    for (size_t i = 0; i < m_Geometry.size(); ++i)
      f(CoordPointT(m_Geometry[i].x, m_Geometry[i].y));
  }

  template <typename FunctorT>
  void ForEachPoint(FunctorT f) const
  {
    ForEachPointRef(f);
  }

  template <typename FunctorT>
  void ForEachTriangleRef(FunctorT & f) const
  {
    if (!m_bTrianglesParsed)
      ParseTriangles();
    for (size_t i = 0; i < m_Triangles.size();)
    {
      f(m_Triangles[i], m_Triangles[i+1], m_Triangles[i+2]);
      i += 3;
    }
  }

  template <typename FunctorT>
  void ForEachTriangleExRef(FunctorT & f) const
  {
    f.StartPrimitive(m_Triangles.size());
    ForEachTriangleRef(f);
    f.EndPrimitive();
  }

protected:

  void ParseGeometry() const;
  void ParseTriangles() const;

  void ParseAll() const;

  mutable vector<m2::PointD> m_Geometry;
  mutable vector<m2::PointD> m_Triangles;
};

class FeatureGeomRef : public FeatureGeom
{
  typedef FeatureGeom base_type;

public:
  FeatureGeomRef() {}
  FeatureGeomRef(vector<char> & data, uint32_t offset = 0) : base_type(data, offset) {}
};

inline string debug_print(FeatureGeom const & f)
{
  return f.DebugString();
}


typedef FeatureGeom FeatureType;
