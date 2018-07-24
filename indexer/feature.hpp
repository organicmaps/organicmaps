#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include <cstdint>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace feature
{
class LoaderBase;
class LoaderCurrent;
}

namespace osm
{
class EditableMapObject;
}

// Lazy feature loader. Loads needed data and caches it.
class FeatureType
{
public:
  using Buffer = char const *;

  feature::EGeomType GetFeatureType() const;
  FeatureParamsBase & GetParams() { return m_params; }

  uint8_t GetTypesCount() const { return (m_header & feature::HEADER_TYPE_MASK) + 1; }

  bool HasName() const { return (m_header & feature::HEADER_HAS_NAME) != 0; }

  void SetTypes(uint32_t const (&types)[feature::kMaxTypesCount], uint32_t count);

  void Deserialize(feature::LoaderBase * loader, Buffer buffer);

  void ParseTypes();
  void ParseCommon();

  m2::PointD GetCenter()
  {
    ASSERT_EQUAL(GetFeatureType(), feature::GEOM_POINT, ());
    ParseCommon();
    return m_center;
  }

  template <class T>
  bool ForEachName(T && fn)
  {
    if (!HasName())
      return false;

    ParseCommon();
    m_params.name.ForEach(forward<T>(fn));
    return true;
  }

  template <typename ToDo>
  void ForEachType(ToDo && f)
  {
    ParseTypes();

    uint32_t const count = GetTypesCount();
    for (size_t i = 0; i < count; ++i)
      f(m_types[i]);
  }

  int8_t GetLayer()
  {
    if (!(m_header & feature::HEADER_HAS_LAYER))
      return 0;

    ParseCommon();
    return m_params.layer;
  }

  /// @name Editor methods.
  //@{
  /// Apply changes from UI for edited or newly created features.
  /// Replaces all FeatureType's components.
  void ReplaceBy(osm::EditableMapObject const & ef);

  StringUtf8Multilang const & GetNames();
  void SetNames(StringUtf8Multilang const & newNames);
  void SetMetadata(feature::Metadata const & newMetadata);

  void UpdateHeader(bool commonParsed, bool metadataParsed);
  bool UpdateMetadataValue(std::string const & key, std::string const & value);
  void ForEachMetadataItem(
      bool skipSponsored,
      std::function<void(std::string const & tag, std::string const & value)> const & fn) const;

  void SetCenter(m2::PointD const &pt);
  //@}

  void SetID(FeatureID const & id) { m_id = id; }
  FeatureID const & GetID() const { return m_id; }

  /// @name Parse functions. Do simple dispatching to m_loader.
  //@{
  /// Super-method to call all possible Parse* methods.
  void ParseEverything();
  void ParseHeader2();

  void ResetGeometry();
  uint32_t ParseGeometry(int scale);
  uint32_t ParseTriangles(int scale);

  void ParseMetadata();
  //@}

  /// @name Geometry.
  //@{
  /// This constant values should be equal with feature::LoaderBase implementation.
  enum { BEST_GEOMETRY = -1, WORST_GEOMETRY = -2 };

  m2::RectD GetLimitRect(int scale);

  bool IsEmptyGeometry(int scale);

  template <typename Functor>
  void ForEachPoint(Functor && f, int scale)
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

  size_t GetPointsCount() const
  {
    ASSERT(m_parsed.m_points, ());
    return m_points.size();
  }

  m2::PointD const & GetPoint(size_t i) const
  {
    ASSERT_LESS(i, m_points.size(), ());
    ASSERT(m_parsed.m_points, ());
    return m_points[i];
  }

  template <typename TFunctor>
  void ForEachTriangle(TFunctor && f, int scale)
  {
    ParseTriangles(scale);

    for (size_t i = 0; i < m_triangles.size();)
    {
      f(m_triangles[i], m_triangles[i+1], m_triangles[i+2]);
      i += 3;
    }
  }

  std::vector<m2::PointD> GetTriangesAsPoints(int scale)
  {
    ParseTriangles(scale);
    return {std::begin(m_triangles), std::end(m_triangles)};
  }

  template <typename Functor>
  void ForEachTriangleEx(Functor && f, int scale) const
  {
    f.StartPrimitive(m_triangles.size());
    ForEachTriangle(forward<Functor>(f), scale);
    f.EndPrimitive();
  }
  //@}

  std::string DebugString(int scale);

  std::string GetHouseNumber();
  /// Needed for Editor, to change house numbers in runtime.
  void SetHouseNumber(std::string const & number);

  /// @name Get names for feature.
  /// @param[out] defaultName corresponds to osm tag "name"
  /// @param[out] intName optionally choosen from tags "name:<lang_code>" by the algorithm
  //@{
  /// Just get feature names.
  void GetPreferredNames(std::string & defaultName, std::string & intName);
  void GetPreferredNames(bool allowTranslit, int8_t deviceLang, std::string & defaultName,
                         std::string & intName);
  /// Get one most suitable name for user.
  void GetReadableName(std::string & name);
  void GetReadableName(bool allowTranslit, int8_t deviceLang, std::string & name);

  static int8_t const DEFAULT_LANG = StringUtf8Multilang::kDefaultCode;
  bool GetName(int8_t lang, std::string & name);
  //@}

  uint8_t GetRank();
  uint64_t GetPopulation();
  std::string GetRoadNumber();

  feature::Metadata & GetMetadata()
  {
    ParseMetadata();
    return m_metadata;
  }

  /// @name Statistic functions.
  //@{
  void ParseBeforeStatistic() { ParseHeader2(); }

  struct InnerGeomStat
  {
    uint32_t m_points, m_strips, m_size;

    void MakeZero()
    {
      m_points = m_strips = m_size = 0;
    }
  };

  InnerGeomStat GetInnerStatistic() const { return m_innerStats; }

  struct GeomStat
  {
    uint32_t m_size, m_count;

    GeomStat(uint32_t sz, size_t count) : m_size(sz), m_count(static_cast<uint32_t>(count)) {}

    GeomStat() : m_size(0), m_count(0) {}
  };

  GeomStat GetGeometrySize(int scale);
  GeomStat GetTrianglesSize(int scale);
  //@}

private:
  struct ParsedFlags
  {
    bool m_types = false;
    bool m_common = false;
    bool m_header2 = false;
    bool m_points = false;
    bool m_triangles = false;
    bool m_metadata = false;
  };

  struct Offsets
  {
    static uint32_t const m_types = 1;
    uint32_t m_common;
    uint32_t m_header2;
  };

  void ParseGeometryAndTriangles(int scale);

  feature::LoaderBase * m_loader;
  Buffer m_data = 0;

  uint8_t m_header = 0;

  uint32_t m_types[feature::kMaxTypesCount];

  FeatureID m_id;
  FeatureParamsBase m_params;

  m2::PointD m_center;
  m2::RectD m_limitRect;

  // For better result this value should be greater than 17
  // (number of points in inner triangle-strips).
  static const size_t static_buffer = 32;
  using Points = buffer_vector<m2::PointD, static_buffer>;
  Points m_points, m_triangles;
  feature::Metadata m_metadata;

  ParsedFlags m_parsed;
  Offsets m_offsets;
  uint32_t m_ptsSimpMask;
  using GeometryOffsets = buffer_vector<uint32_t, feature::DataHeader::MAX_SCALES_COUNT>;
  GeometryOffsets m_ptsOffsets, m_trgOffsets;

  InnerGeomStat m_innerStats;

  friend class feature::LoaderCurrent;
  friend class feature::LoaderBase;
};

namespace feature
{
template <class Iter>
void CalcRect(Iter b, Iter e, m2::RectD & rect)
{
  while (b != e)
    rect.Add(*b++);
}

template <class Cont>
void CalcRect(Cont const & points, m2::RectD & rect)
{
  CalcRect(points.begin(), points.end(), rect);
}
}
