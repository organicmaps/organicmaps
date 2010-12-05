#pragma once

#include "cell_id.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../base/base.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/iterator.hpp"
#include "../std/algorithm.hpp"

class Feature;

class FeatureBuilder
{
public:
  FeatureBuilder();
  void AddName(string const & name);
  void AddPoint(m2::PointD const & p);
  void AddTriangle(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c);

  template <class TIter>
  void AddTypes(TIter beg, TIter end)
  {
    // 15 - is the maximum count of types (@see Feature::GetTypesCount())
    size_t const count = min(15, static_cast<int>(distance(beg, end)));
    m_Types.assign(beg, beg + count);
  }
  void SetType(uint32_t type)
  {
    m_Types.clear();
    m_Types.push_back(type);
  }

  void AddLayer(int32_t layer);

  void Serialize(vector<char> & data) const;

  // Returns corresponding feature. Does Serialize() and feature.Deserialize() internally.
  Feature GetFeature() const;

  bool IsGeometryClosed() const;
  size_t GetPointsCount() const { return m_Geometry.size(); }

  bool operator == (FeatureBuilder const &) const;

private:
  int32_t m_Layer;
  string m_Name;
  vector<uint32_t> m_Types;
  vector<int64_t> m_Geometry;
  vector<int64_t> m_Triangles;
};

class Feature
{
public:
  enum FeatureType
  {
    FEATURE_TYPE_POINT = 0,
    FEATURE_TYPE_LINE = 1,
    FEATURE_TYPE_AREA = 2,
    FEATURE_TYPE_UNKNOWN = 17
  };

  Feature() {}
  Feature(vector<char> & data, uint32_t offset = 0);

  void Deserialize(vector<char> & data, uint32_t offset = 0);
  void DeserializeAndParse(vector<char> & data, uint32_t offset = 0);

  string DebugString() const;

  FeatureBuilder GetFeatureBuilder() const;

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

  inline m2::RectD GetLimitRect() const
  {
    if (!m_bGeometryParsed)
      ParseGeometry();
    return m_LimitRect;
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
    return m_TriangleCount;
  }

  inline string GetName() const
  {
    if (!(Header() & HEADER_HAS_NAME))
      return string();
    if (!m_bNameParsed)
      ParseName();
    return m_Name;
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

  enum
  {
    HEADER_HAS_LAYER = 1U << 7,
    HEADER_HAS_NAME = 1U << 6,
    HEADER_IS_AREA = 1U << 5,
    HEADER_IS_LINE = 1U << 4
  };

private:
  vector<char> m_Data;
  uint32_t m_Offset;

  inline char const * DataPtr() const { return &m_Data[m_Offset]; }
  inline uint8_t Header() const { return static_cast<uint8_t>(*DataPtr()); }

  mutable uint32_t m_Types[16];
  mutable int32_t m_Layer;
  mutable vector<m2::PointD> m_Geometry;
  mutable m2::RectD m_LimitRect;
  mutable vector<m2::PointD> m_Triangles;
  mutable uint32_t m_TriangleCount;
  mutable string m_Name;

  mutable uint32_t m_LayerOffset;
  mutable uint32_t m_GeometryOffset;
  mutable uint32_t m_TrianglesOffset;
  mutable uint32_t m_NameOffset;

  mutable bool m_bTypesParsed;
  mutable bool m_bLayerParsed;
  mutable bool m_bGeometryParsed;
  mutable bool m_bTrianglesParsed;
  mutable bool m_bNameParsed;

  void ParseTypes() const;
  void ParseLayer() const;
  void ParseGeometry() const;
  void ParseTriangles() const;
  void ParseName() const;
  void ParseAll() const;
};

inline string debug_print(Feature const & f)
{
  return f.DebugString();
}
