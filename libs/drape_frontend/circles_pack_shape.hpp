#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/overlay_handle.hpp"
#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"
#include "drape/utils/vertex_decl.hpp"

#include <vector>

namespace dp
{
class TextureManager;
class GraphicsContext;
}  // namespace dp

namespace df
{
struct CirclesPackRenderData
{
  uint32_t m_pointsCount;

  dp::RenderState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;
  CirclesPackRenderData()
    : m_pointsCount(0)
    , m_state(CreateRenderState(gpu::Program::CirclePoint, DepthLayer::OverlayLayer))
  {}
};

struct CirclesPackDynamicVertex
{
  using TPosition = glsl::vec3;
  using TColor = glsl::vec4;

  CirclesPackDynamicVertex() = default;
  CirclesPackDynamicVertex(TPosition const & pos, TColor const & color) : m_position(pos), m_color(color) {}

  TPosition m_position;
  TColor m_color;
};

class CirclesPackHandle : public dp::OverlayHandle
{
  using TBase = dp::OverlayHandle;

public:
  explicit CirclesPackHandle(size_t pointsCount);
  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const override;
  bool Update(ScreenBase const & screen) override;
  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override;
  void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const override;
  bool IndexesRequired() const override;

  void Clear();
  void SetPoint(size_t index, m2::PointD const & position, float radius, dp::Color const & color);
  size_t GetPointsCount() const;

private:
  std::vector<CirclesPackDynamicVertex> m_buffer;
  mutable bool m_needUpdate;
};

class CirclesPackShape
{
public:
  static void Draw(ref_ptr<dp::GraphicsContext> context, CirclesPackRenderData & data);
};
}  // namespace df
