#pragma once

#include "drape_frontend/batchers_pool.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/color.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glstate.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/render_bucket.hpp"
#include "drape/texture_manager.hpp"

#include "traffic/traffic_info.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/polyline2d.hpp"

#include "std/array.hpp"
#include "std/map.hpp"
#include "std/set.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/unordered_map.hpp"

namespace df
{

enum class RoadClass : uint8_t
{
  Class0,
  Class1,
  Class2
};

struct TrafficSegmentID
{
  MwmSet::MwmId m_mwmId;
  traffic::TrafficInfo::RoadSegmentId m_segmentId;

  TrafficSegmentID(MwmSet::MwmId const & mwmId,
                   traffic::TrafficInfo::RoadSegmentId const & segmentId)
    : m_mwmId(mwmId)
    , m_segmentId(segmentId)
  {}

  inline bool operator<(TrafficSegmentID const & r) const
  {
    if (m_mwmId == r.m_mwmId)
      return m_segmentId < r.m_segmentId;
    return m_mwmId < r.m_mwmId;
  }

  inline bool operator==(TrafficSegmentID const & r) const
  {
    return (m_mwmId == r.m_mwmId && m_segmentId == r.m_segmentId);
  }

  inline bool operator!=(TrafficSegmentID const & r) const { return !(*this == r); }
};

struct TrafficSegmentGeometry
{
  m2::PolylineD m_polyline;
  RoadClass m_roadClass;

  TrafficSegmentGeometry(m2::PolylineD && polyline, RoadClass const & roadClass)
    : m_polyline(move(polyline))
    , m_roadClass(roadClass)
  {}
};

using TrafficSegmentsGeometry = map<TrafficSegmentID, TrafficSegmentGeometry>;

struct TrafficSegmentColoring
{
  TrafficSegmentID m_id;
  traffic::SpeedGroup m_speedGroup;

  TrafficSegmentColoring(TrafficSegmentID const & id, traffic::SpeedGroup const & speedGroup)
    : m_id(id)
    , m_speedGroup(speedGroup)
  {}
};

using TrafficSegmentsColoring = vector<TrafficSegmentColoring>;

struct TrafficRenderData
{
  dp::GLState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;
  TileKey m_tileKey;
  MwmSet::MwmId m_mwmId;
  m2::RectD m_boundingBox;
  TrafficRenderData(dp::GLState const & state) : m_state(state) {}
};

struct TrafficStaticVertex
{
  using TPosition = glsl::vec3;
  using TNormal = glsl::vec4;

  TrafficStaticVertex() = default;
  TrafficStaticVertex(TPosition const & position, TNormal const & normal)
    : m_position(position)
    , m_normal(normal)
  {}

  TPosition m_position;
  TNormal m_normal;
};

struct TrafficDynamicVertex
{
  using TTexCoord = glsl::vec2;

  TrafficDynamicVertex() = default;
  TrafficDynamicVertex(TTexCoord const & color)
    : m_colorTexCoord(color)
  {}

  TTexCoord m_colorTexCoord;
};

class TrafficHandle : public dp::OverlayHandle
{
  using TBase = dp::OverlayHandle;

public:
  TrafficHandle(TrafficSegmentID const & segmentId, RoadClass const & roadClass,
                m2::RectD const & boundingBox, glsl::vec2 const & texCoord,
                size_t verticesCount);

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const override;
  bool Update(ScreenBase const & screen) override;
  bool IndexesRequired() const override;
  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override;
  void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const override;

  void SetTexCoord(glsl::vec2 const & texCoord);
  TrafficSegmentID GetSegmentId() const;
  RoadClass const & GetRoadClass() const;
  m2::RectD const & GetBoundingBox() const;

private:
  TrafficSegmentID m_segmentId;
  RoadClass m_roadClass;
  vector<glsl::vec2> m_buffer;
  m2::RectD m_boundingBox;
  mutable bool m_needUpdate;
};

using TrafficTexCoords = unordered_map<size_t, glsl::vec2>;

class TrafficGenerator final
{
public:
  using TFlushRenderDataFn = function<void (TrafficRenderData && renderData)>;

  explicit TrafficGenerator(TFlushRenderDataFn flushFn)
    : m_flushRenderDataFn(flushFn)
  {}

  void Init();
  void ClearGLDependentResources();

  void AddSegmentsGeometry(TrafficSegmentsGeometry const & geom);

  TrafficSegmentsColoring GetSegmentsToUpdate(TrafficSegmentsColoring const & trafficColoring) const;

  void GetTrafficGeom(ref_ptr<dp::TextureManager> textures,
                      TrafficSegmentsColoring const & trafficColoring);

  void ClearCache();
  void ClearCache(MwmSet::MwmId const & mwmId);

  bool IsColorsCacheRefreshed() const { return m_colorsCacheRefreshed; }
  TrafficTexCoords ProcessCacheRefreshing();

private:
  struct TrafficBatcherKey
  {
    TrafficBatcherKey() = default;
    TrafficBatcherKey(MwmSet::MwmId const & mwmId, TileKey const & tileKey, RoadClass const & roadClass)
      : m_mwmId(mwmId)
      , m_tileKey(tileKey)
      , m_roadClass(roadClass)
    {}

    MwmSet::MwmId m_mwmId;
    TileKey m_tileKey;
    RoadClass m_roadClass;
  };

  struct TrafficBatcherKeyComparator
  {
    bool operator() (TrafficBatcherKey const & lhs, TrafficBatcherKey const & rhs) const
    {
      if (lhs.m_mwmId == rhs.m_mwmId)
      {
        if (lhs.m_tileKey == rhs.m_tileKey)
          return lhs.m_roadClass < rhs.m_roadClass;
        return lhs.m_tileKey < rhs.m_tileKey;
      }
      return lhs.m_mwmId < rhs.m_mwmId;
    }
  };

  void GenerateSegment(dp::TextureManager::ColorRegion const & colorRegion,
                       m2::PolylineD const & polyline, m2::PointD const & tileCenter,
                       vector<TrafficStaticVertex> & staticGeometry,
                       vector<TrafficDynamicVertex> & dynamicGeometry);
  void FillColorsCache(ref_ptr<dp::TextureManager> textures);

  void FlushGeometry(TrafficBatcherKey const & key, dp::GLState const & state, drape_ptr<dp::RenderBucket> && buffer);

  TrafficSegmentsGeometry m_segments;

  set<TrafficSegmentID> m_segmentsCache;
  array<dp::TextureManager::ColorRegion, static_cast<size_t>(traffic::SpeedGroup::Count)> m_colorsCache;
  bool m_colorsCacheValid = false;
  bool m_colorsCacheRefreshed = false;

  drape_ptr<BatchersPool<TrafficBatcherKey, TrafficBatcherKeyComparator>> m_batchersPool;
  TFlushRenderDataFn m_flushRenderDataFn;
};

} // namespace df
