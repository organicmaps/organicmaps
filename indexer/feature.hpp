#pragma once
#include "indexer/feature_data.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/macros.hpp"

#include <array>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

namespace feature
{
class SharedLoadInfo;
struct NameParamsOut; // Include feature_utils.hpp when using
}

namespace osm
{
class MapObject;
}

// Lazy feature loader. Loads needed data and caches it.
class FeatureType
{
  FeatureType() = default;

public:
  using GeometryOffsets = buffer_vector<uint32_t, feature::DataHeader::kMaxScalesCount>;

  FeatureType(feature::SharedLoadInfo const * loadInfo, std::vector<uint8_t> && buffer,
              indexer::MetadataDeserializer * metadataDeserializer);

  static std::unique_ptr<FeatureType> CreateFromMapObject(osm::MapObject const & emo);

  feature::GeomType GetGeomType() const;

  uint8_t GetTypesCount() const { return (m_header & feature::HEADER_MASK_TYPE) + 1; }

  bool HasName() const { return (m_header & feature::HEADER_MASK_HAS_NAME) != 0; }
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

  std::vector<m2::PointD> GetTrianglesAsPoints(int scale);

  void SetID(FeatureID id) { m_id = std::move(id); }
  FeatureID const & GetID() const { return m_id; }

  void ParseHeader2();
  void ResetGeometry();
  void ParseGeometry(int scale);
  void ParseTriangles(int scale);
  //@}

  /// @name Geometry.
  //@{
  /// This constant values should be equal with feature::FeatureLoader implementation.
  enum { BEST_GEOMETRY = -1, WORST_GEOMETRY = -2 };

  m2::RectD GetLimitRect(int scale);
  m2::RectD const & GetLimitRectChecked() const;

  bool IsEmptyGeometry(int scale);

  template <typename Functor>
  void ForEachPoint(Functor && f, int scale)
  {
    ParseGeometry(scale);

    if (m_points.empty())
    {
      // it's a point feature
      if (GetGeomType() == feature::GeomType::Point)
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
  void ForEachTriangleEx(Functor && f, int scale)
  {
    f.StartPrimitive(m_triangles.size());
    ForEachTriangle(std::forward<Functor>(f), scale);
    f.EndPrimitive();
  }
  //@}

  std::string DebugString(int scale);

  std::string const & GetHouseNumber();

  /// @name Get names for feature.
  //@{
  /// @return {primary, secondary} names
  std::pair<std::string_view, std::string_view> GetPreferredNames();
  void GetPreferredNames(bool allowTranslit, int8_t deviceLang, feature::NameParamsOut & out);

  /// Get one most suitable name for user.
  std::string_view GetReadableName();
  void GetReadableName(bool allowTranslit, int8_t deviceLang, feature::NameParamsOut & out);

  std::string_view GetName(int8_t lang);
  //@}

  uint8_t GetRank();
  uint64_t GetPopulation();
  std::string const & GetRoadNumber();

  feature::Metadata const & GetMetadata();

  // Gets single metadata string. Does not parse all metadata.
  std::string_view GetMetadata(feature::Metadata::EType type);
  bool HasMetadata(feature::Metadata::EType type);

  /// @name Stats functions.
  //@{
  struct InnerGeomStat
  {
    uint32_t m_points = 0, m_strips = 0, m_size = 0;
  };

  InnerGeomStat GetInnerStats() const { return m_innerStats; }

  using GeomArr = uint32_t[feature::DataHeader::kMaxScalesCount];
  struct GeomStat
  {
    GeomArr m_sizes = {}, m_elements = {};
  };

  // Returns outer points/triangles stats for all geo levels and loads the best geometry.
  GeomStat GetOuterGeometryStats();
  GeomStat GetOuterTrianglesStats();
  //@}

private:
  struct ParsedFlags
  {
    bool m_types : 1;
    bool m_common : 1;
    bool m_header2 : 1;
    bool m_points : 1;
    bool m_triangles : 1;
    bool m_metadata : 1;
    bool m_metaIds : 1;

    ParsedFlags() { Reset(); }
    void Reset()
    {
      m_types = m_common = m_header2 = m_points = m_triangles = m_metadata = m_metaIds = false;
    }
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
  void ParseMetadata();
  void ParseMetaIds();
  void ParseGeometryAndTriangles(int scale);

  uint8_t m_header = 0;
  std::array<uint32_t, feature::kMaxTypesCount> m_types = {};

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
  indexer::MetadataDeserializer::MetaIds m_metaIds;

  // Non-owning pointer to shared load info. SharedLoadInfo created once per FeaturesVector.
  feature::SharedLoadInfo const * m_loadInfo = nullptr;
  std::vector<uint8_t> m_data;

  // Pointer to shared metedata deserializer. Must be set for mwm format >= Format::v11
  indexer::MetadataDeserializer * m_metadataDeserializer = nullptr;

  ParsedFlags m_parsed;
  Offsets m_offsets;
  uint32_t m_ptsSimpMask = 0;

  InnerGeomStat m_innerStats;

  DISALLOW_COPY_AND_MOVE(FeatureType);
};
