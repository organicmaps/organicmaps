#pragma once

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

using TrafficSegmentsGeometry = vector<pair<TrafficSegmentID, m2::PolylineD>>;

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
  TrafficHandle(TrafficSegmentID const & segmentId, glsl::vec2 const & texCoord, size_t verticesCount);

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const override;
  bool Update(ScreenBase const & screen) override;
  bool IndexesRequired() const override;
  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override;
  void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const override;

  void SetTexCoord(glsl::vec2 const & texCoord);
  TrafficSegmentID GetSegmentId() const;

private:
  TrafficSegmentID m_segmentId;
  vector<glsl::vec2> m_buffer;
  mutable bool m_needUpdate;
};

using TrafficTexCoords = unordered_map<size_t, glsl::vec2>;

class TrafficGenerator final
{
public:
  TrafficGenerator() = default;

  void AddSegment(TrafficSegmentID const & segmentId, m2::PolylineD const & polyline);

  TrafficSegmentsColoring GetSegmentsToUpdate(TrafficSegmentsColoring const & trafficColoring) const;

  void GetTrafficGeom(ref_ptr<dp::TextureManager> textures,
                      TrafficSegmentsColoring const & trafficColoring,
                      vector<TrafficRenderData> & data);

  void ClearCache();

  bool IsColorsCacheRefreshed() const { return m_colorsCacheRefreshed; }
  TrafficTexCoords ProcessCacheRefreshing();

private:
  using TSegmentCollection = map<TrafficSegmentID, m2::PolylineD>;

  void GenerateSegment(dp::TextureManager::ColorRegion const & colorRegion,
                       m2::PolylineD const & polyline, m2::PointD const & tileCenter,
                       vector<TrafficStaticVertex> & staticGeometry,
                       vector<TrafficDynamicVertex> & dynamicGeometry);
  void FillColorsCache(ref_ptr<dp::TextureManager> textures);

  TSegmentCollection m_segments;

  set<TrafficSegmentID> m_segmentsCache;
  array<dp::TextureManager::ColorRegion, static_cast<size_t>(traffic::SpeedGroup::Count)> m_colorsCache;
  bool m_colorsCacheValid = false;
  bool m_colorsCacheRefreshed = false;
};

} // namespace df
