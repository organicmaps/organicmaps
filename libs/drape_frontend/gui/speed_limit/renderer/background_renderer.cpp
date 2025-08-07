#include "background_renderer.hpp"

#include "base/type_tag.hpp"

#include <utility>

namespace gui::speed_limit::renderer
{
BackgroundVertexData createCircleVertexData()
{
  BackgroundVertexData data;
  data.emplace_back(glsl::vec2(-1.0, 1.0));
  data.emplace_back(glsl::vec2(-1.0, -1.0));
  data.emplace_back(glsl::vec2(1.0, 1.0));
  data.emplace_back(glsl::vec2(1.0, -1.0));
  return data;
}

BackgroundHandle::BackgroundHandle(m2::PointF const & pivot, BackgroundVertexData data)
  : Handle(base::TypeTag<BackgroundHandle>::id<std::uint32_t>(), dp::Center, pivot)
{
  SetIsVisible(true);
  auto const config = DrapeGui::GetSpeedLimitHelper().GetConfig();
  m_params.m_length = DrapeGui::GetSpeedLimitHelper().GetSize();

  m_params.m_color = glsl::ToVec3(config.backgroundColor);
  m_params.m_outlineColor = glsl::ToVec3(config.backgroundOutlineColor);
  m_params.m_edgeColor = glsl::ToVec3(config.backgroundEdgeColor);

  m_params.m_outlineWidthRatio = config.outlineWidthRatio;
  m_params.m_edgeWidthRatio = config.edgeWidthRatio;
}

void BackgroundRenderer::SetPosition(Position const & position)
{
  m_position = position;
}

void BackgroundRenderer::SetRadius(float size)
{
  m_size = size;
}

void BackgroundRenderer::Draw(ref_ptr<dp::GraphicsContext> context, ShapeControl & control) const
{
  BackgroundVertexData data = createCircleVertexData();

  auto state = df::CreateRenderState(gpu::Program::GuiSpeedLimit, df::DepthLayer::GuiLayer);
  state.SetDepthTestEnabled(false);

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, BackgroundVertex::GetBindingInfo(), make_ref(data.data()));
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<BackgroundHandle>(m_position.m_pixelPivot, data);

  dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
  batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
  dp::SessionGuard guard(context, batcher, std::bind(&ShapeControl::AddShape, &control, _1, _2));
  batcher.InsertTriangleStrip(context, state, make_ref(&provider), std::move(handle));
}
}  // namespace gui::speed_limit::renderer
