#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace feature
{
class SharedLoadInfo;
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
  using GeometryOffsets = buffer_vector<uint32_t, feature::DataHeader::MAX_SCALES_COUNT>;

  void Deserialize(feature::SharedLoadInfo const * loadInfo, Buffer buffer);

  feature::EGeomType GetFeatureType() const;
  FeatureParamsBase & GetParams() { return m_params; }

  uint8_t GetTypesCount() const { return (m_header & feature::HEADER_TYPE_MASK) + 1; }

  bool HasName() const { return (m_header & feature::HEADER_HAS_NAME) != 0; }
  StringUtf8Multilang const & GetNames();

  m2::PointD GetCenter();

  template <class T>
  bool ForEachName(T && fn)
  {
    if (!HasName())
      return false;

    ParseCommon();
    m_params.name.ForEach(std::forward<T>(fn));
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

  int8_t GetLayer();

  /// @name Editor methods.
  //@{
  /// Apply changes from UI for edited or newly created features.
  /// Replaces all FeatureType's components.
  std::vector<m2::PointD> GetTriangesAsPoints(int scale);

  void ReplaceBy(osm::EditableMapObject const & ef);

  void SetNames(StringUtf8Multilang const & newNames);
  void SetTypes(std::array<uint32_t, feature::kMaxTypesCount> const & types, uint32_t count);
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

  /// @name Parse functions.
  //@{
  /// Super-method to call all possible Parse* methods.
  void ParseEverything();

  void ResetGeometry();
  uint32_t ParseGeometry(int scale);
  uint32_t ParseTriangles(int scale);
  //@}

  /// @name Geometry.
  //@{
  /// This constant values should be equal with feature::FeatureLoader implementation.
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

  size_t GetPointsCount() const;

  m2::PointD const & GetPoint(size_t i) const;

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

  template <typename Functor>
  void ForEachTriangleEx(Functor && f, int scale) const
  {
    f.StartPrimitive(m_triangles.size());
    ForEachTriangle(std::forward<Functor>(f), scale);
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

  bool GetName(int8_t lang, std::string & name);
  //@}

  uint8_t GetRank();
  uint64_t GetPopulation();
  std::string GetRoadNumber();

  feature::Metadata & GetMetadata();

  /// @name Statistic functions.
  //@{
  void ParseBeforeStatistic() { ParseHeader2(); }

  struct InnerGeomStat
  {
    uint32_t m_points = 0, m_strips = 0, m_size = 0;

    void MakeZero()
    {
      m_points = m_strips = m_size = 0;
    }
  };

  InnerGeomStat GetInnerStatistic() const { return m_innerStats; }

  struct GeomStat
  {
    uint32_t m_size = 0, m_count = 0;

    GeomStat(uint32_t sz, size_t count) : m_size(sz), m_count(static_cast<uint32_t>(count)) {}
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

    void Reset() { m_types = m_common = m_header2 = m_points = m_triangles = m_metadata = false; }
  };

  struct Offsets
  {
    uint32_t m_common = 0;
    uint32_t m_header2 = 0;
    GeometryOffsets m_pts;
    GeometryOffsets m_trg;

    void Reset()
    {
      m_common = m_header2 = 0;
      m_pts.clear();
      m_trg.clear();
    }
  };

  void ParseTypes();
  void ParseCommon();
  void ParseHeader2();
  void ParseMetadata();
  void ParseGeometryAndTriangles(int scale);

  uint8_t m_header = 0;
  std::array<uint32_t, feature::kMaxTypesCount> m_types;

  FeatureID m_id;
  FeatureParamsBase m_params;

  m2::PointD m_center;
  m2::RectD m_limitRect;

  // For better result this value should be greater than 17
  // (number of points in inner triangle-strips).
  static const size_t kStaticBufferSize = 32;
  using Points = buffer_vector<m2::PointD, kStaticBufferSize>;
  Points m_points, m_triangles;
  feature::Metadata m_metadata;

  // Non-owning pointer to shared load info. SharedLoadInfo created once per FeaturesVector.
  feature::SharedLoadInfo const * m_loadInfo = nullptr;
  // Raw pointer to data buffer.
  Buffer m_data = nullptr;

  ParsedFlags m_parsed;
  Offsets m_offsets;
  uint32_t m_ptsSimpMask = 0;

  InnerGeomStat m_innerStats;
};
