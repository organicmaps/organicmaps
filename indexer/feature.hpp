#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/feature_data.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include "std/string.hpp"


namespace feature
{
  class LoaderBase;
  class LoaderCurrent;
}

namespace old_101 { namespace feature
{
  class LoaderImpl;
}}


/// Base feature class for storing common data (without geometry).
class FeatureBase
{
  static const int m_maxTypesCount = feature::max_types_count;

public:

  using TBuffer = char const *;

  void Deserialize(feature::LoaderBase * pLoader, TBuffer buffer);

  /// @name Parse functions. Do simple dispatching to m_pLoader.
  //@{
  void ParseTypes() const;
  void ParseCommon() const;
  //@}

  feature::EGeomType GetFeatureType() const;

  inline uint8_t GetTypesCount() const
  {
    return ((Header() & feature::HEADER_TYPE_MASK) + 1);
  }

  inline int8_t GetLayer() const
  {
    if (!(Header() & feature::HEADER_HAS_LAYER))
      return 0;

    ParseCommon();
    return m_params.layer;
  }

  inline bool HasName() const
  {
    return (Header() & feature::HEADER_HAS_NAME) != 0;
  }

  // Array with 64 strings ??? Use ForEachName instead!
  /*
  class GetNamesFn
  {
  public:
    string m_names[64];
    char m_langs[64];
    size_t m_size;

    GetNamesFn() : m_size(0) {}
    bool operator() (char lang, string const & name)
    {
      m_names[m_size++] = name;
      m_langs[m_size] = lang;
      return true;
    }
  };
  */

  template <class T>
  inline bool ForEachNameRef(T & functor) const
  {
    if (!HasName())
      return false;

    ParseCommon();
    m_params.name.ForEachRef(functor);
    return true;
  }

  inline m2::RectD GetLimitRect() const
  {
    ASSERT ( m_limitRect.IsValid(), () );
    return m_limitRect;
  }

  inline m2::PointD GetCenter() const
  {
    ASSERT_EQUAL ( GetFeatureType(), feature::GEOM_POINT, () );
    ParseCommon();
    return m_center;
  }

  template <typename ToDo>
  void ForEachType(ToDo f) const
  {
    ParseTypes();

    uint32_t const count = GetTypesCount();
    for (size_t i = 0; i < count; ++i)
      f(m_types[i]);
  }

protected:
  /// @name Need for FeatureBuilder.
  //@{
  friend class FeatureBuilder1;
  inline void SetHeader(uint8_t h) { m_header = h; }
  //@}

  string DebugString() const;

  inline uint8_t Header() const { return m_header; }

protected:
  feature::LoaderBase * m_pLoader;

  uint8_t m_header;

  mutable uint32_t m_types[m_maxTypesCount];

  mutable FeatureParamsBase m_params;

  mutable m2::PointD m_center;

  mutable m2::RectD m_limitRect;

  mutable bool m_bTypesParsed, m_bCommonParsed;

  friend class feature::LoaderCurrent;
  friend class old_101::feature::LoaderImpl;
};

/// Working feature class with geometry.
class FeatureType : public FeatureBase
{
  typedef FeatureBase base_type;

  FeatureID m_id;

public:
  void Deserialize(feature::LoaderBase * pLoader, TBuffer buffer);

  inline void SetID(FeatureID const & id) { m_id = id; }
  inline FeatureID GetID() const { return m_id; }

  /// @name Parse functions. Do simple dispatching to m_pLoader.
  //@{
  void ParseHeader2() const;

  void ResetGeometry() const;
  uint32_t ParseGeometry(int scale) const;
  uint32_t ParseTriangles(int scale) const;

  void ParseMetadata() const;
  //@}

  /// @name Geometry.
  //@{
  /// This constant values should be equal with feature::LoaderBase implementation.
  enum { BEST_GEOMETRY = -1, WORST_GEOMETRY = -2 };

  m2::RectD GetLimitRect(int scale) const;

  bool IsEmptyGeometry(int scale) const;

  template <typename FunctorT>
  void ForEachPointRef(FunctorT & f, int scale) const
  {
    ParseGeometry(scale);

    if (m_points.empty())
    {
      // it's a point feature
      if (GetFeatureType() == feature::GEOM_POINT)
        f(m_center);
    }
    else
    {
      for (size_t i = 0; i < m_points.size(); ++i)
        f(m_points[i]);
    }
  }

  inline size_t GetPointsCount() const
  {
    ASSERT(m_bPointsParsed, ());
    return m_points.size();
  }
  inline m2::PointD const & GetPoint(size_t i) const
  {
    ASSERT_LESS(i, m_points.size(), ());
    ASSERT(m_bPointsParsed, ());
    return m_points[i];
  }

  template <typename FunctorT>
  void ForEachPoint(FunctorT f, int scale) const
  {
    ForEachPointRef(f, scale);
  }

  template <typename FunctorT>
  void ForEachTriangleRef(FunctorT & f, int scale) const
  {
    ParseTriangles(scale);

    for (size_t i = 0; i < m_triangles.size();)
    {
      f(m_triangles[i], m_triangles[i+1], m_triangles[i+2]);
      i += 3;
    }
  }

  template <typename FunctorT>
  void ForEachTriangle(FunctorT f, int scale) const
  {
    ForEachTriangleRef(f, scale);
  }

  template <typename FunctorT>
  void ForEachTriangleExRef(FunctorT & f, int scale) const
  {
    f.StartPrimitive(m_triangles.size());
    ForEachTriangleRef(f, scale);
    f.EndPrimitive();
  }
  //@}

  /// For test cases only.
  string DebugString(int scale) const;
  friend string DebugPrint(FeatureType const & ft);

  string GetHouseNumber() const;

  /// @name Get names for feature.
  /// @param[out] defaultName corresponds to osm tag "name"
  /// @param[out] intName optionally choosen from tags "name:<lang_code>" by the algorithm
  //@{
  /// Just get feature names.
  void GetPreferredNames(string & defaultName, string & intName) const;
  /// Get one most suitable name for user.
  void GetReadableName(string & name) const;

  static int8_t const DEFAULT_LANG = StringUtf8Multilang::DEFAULT_CODE;
  bool GetName(int8_t lang, string & name) const;
  //@}

  uint8_t GetRank() const;
  uint32_t GetPopulation() const;
  string GetRoadNumber() const;
  bool HasInternet() const;

  inline feature::Metadata const & GetMetadata() const { return m_metadata; }
  inline feature::Metadata & GetMetadata() { return m_metadata; }

  double GetDistance(m2::PointD const & pt, int scale) const;

  /// @name Statistic functions.
  //@{
  inline void ParseBeforeStatistic() const
  {
    ParseHeader2();
  }

  struct inner_geom_stat_t
  {
    uint32_t m_points, m_strips, m_size;

    void MakeZero()
    {
      m_points = m_strips = m_size = 0;
    }
  };

  inner_geom_stat_t GetInnerStatistic() const { return m_innerStats; }

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

  void SwapGeometry(FeatureType & r);

  inline void SwapPoints(buffer_vector<m2::PointD, 32> & points) const
  {
    ASSERT(m_bPointsParsed, ());
    return m_points.swap(points);
  }

private:
  void ParseAll(int scale) const;

  // For better result this value should be greater than 17
  // (number of points in inner triangle-strips).
  static const size_t static_buffer = 32;

  typedef buffer_vector<m2::PointD, static_buffer> points_t;
  mutable points_t m_points, m_triangles;
  mutable feature::Metadata m_metadata;

  mutable bool m_bHeader2Parsed, m_bPointsParsed, m_bTrianglesParsed, m_bMetadataParsed;

  mutable inner_geom_stat_t m_innerStats;

  friend class feature::LoaderCurrent;
  friend class old_101::feature::LoaderImpl;
};

namespace feature
{
  template <class IterT>
  void CalcRect(IterT b, IterT e, m2::RectD & rect)
  {
    while (b != e)
      rect.Add(*b++);
  }

  template <class TCont>
  void CalcRect(TCont const & points, m2::RectD & rect)
  {
    CalcRect(points.begin(), points.end(), rect);
  }
}
