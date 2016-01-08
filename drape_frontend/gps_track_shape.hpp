#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/glstate.hpp"
#include "drape/render_bucket.hpp"
#include "drape/utils/vertex_decl.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/pointers.hpp"

#include "std/vector.hpp"

namespace dp
{
  class TextureManager;
}

namespace df
{

struct GpsTrackRenderData
{
  size_t m_pointsCount;

  dp::GLState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;
  GpsTrackRenderData() : m_pointsCount(0), m_state(0, dp::GLState::OverlayLayer) {}
};

struct GpsTrackDynamicVertex
{
  using TPosition = glsl::vec3;
  using TColor = glsl::vec4;

  GpsTrackDynamicVertex() = default;
  GpsTrackDynamicVertex(TPosition const & pos, TColor const & color)
    : m_position(pos)
    , m_color(color)
  {}

  TPosition m_position;
  TColor m_color;
};

class GpsTrackHandle : public dp::OverlayHandle
{
  using TBase = dp::OverlayHandle;

public:
  GpsTrackHandle(size_t pointsCount);
  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                            ScreenBase const & screen) const override;
  bool Update(ScreenBase const & screen) override;
  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override;
  void GetPixelShape(ScreenBase const & screen, Rects & rects, bool perspective) const override;
  bool IndexesRequired() const override;

  void Clear();
  void SetPoint(size_t index, m2::PointD const & position, float radius, dp::Color const & color);
  size_t GetPointsCount() const;

private:
  vector<GpsTrackDynamicVertex> m_buffer;
  bool m_needUpdate;
};

class GpsTrackShape
{
public:
  static void Draw(ref_ptr<dp::TextureManager> texMng, GpsTrackRenderData & data);
};

} // namespace df
