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
    size_t m_size;

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

  /// @name Statistic functions.
  //@{
  uint32_t GetTypesSize() const { return m_CommonOffset - m_TypesOffset; }
  //@}

  FeatureParams GetFeatureParams() const;
  inline m2::PointD GetCenter() const { return m_Center; }

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
  string GetPreferredDrawableName(int8_t const * priorities = NULL) const;

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

  uint32_t GetPopulation() const;
  double GetPopulationDrawRank() const;
  uint8_t GetSearchRank() const;

  string GetRoadNumber() const { return m_Params.ref; }

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
