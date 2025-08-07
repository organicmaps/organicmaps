#include "background_renderer.hpp"

#include "base/type_tag.hpp"

#include <utility>

namespace gui::speed_limit::renderer
{
namespace vertex
{
struct BackgroundVertex
{
  glsl::vec2 m_position;

  static dp::BindingInfo GetBindingInfo()
  {
    dp::BindingFiller<BackgroundVertex> filler(1);
    filler.FillDecl<glsl::vec2>("a_position");
    return filler.m_info;
  }
};

using BackgroundVertexData = buffer_vector<BackgroundVertex, dp::Batcher::VertexPerQuad>;

BackgroundVertexData createVertexData()
{
  BackgroundVertexData data;
  data.emplace_back(glsl::vec2(-1.0, 1.0));
  data.emplace_back(glsl::vec2(-1.0, -1.0));
  data.emplace_back(glsl::vec2(1.0, 1.0));
  data.emplace_back(glsl::vec2(1.0, -1.0));
  return data;
}
}  // namespace vertex

namespace handle
{
class BackgroundHandle : public Handle
{
public:
  explicit BackgroundHandle(RenderingHelperPtr helper, ref_ptr<dp::TextureManager> tex)
    : Handle(base::TypeTag<BackgroundHandle>::id<std::uint32_t>(), dp::Center, m2::PointF::Zero())
    , m_helper(std::move(helper))
    , m_tex(std::move(tex))
  {}

  bool IsTapped(m2::RectD const & touchArea) const override
  {
    if (!IsVisible())
      return false;

    SpeedLimit const & helper = DrapeGui::GetSpeedLimit();
    auto const m_size = helper.GetSize();
    m2::RectD rect(m_pivot.x - m_size, m_pivot.y - m_size, m_pivot.x + m_size, m_pivot.y + m_size);
    return rect.Intersect(touchArea);
  }

  bool Update(ScreenBase const & screen) override
  {
    SetIsVisible(m_helper->IsVisible());

    if (m_helper->IsBackgroundUpdateNeeded())
    {
      m_pivot = glsl::ToVec2(m_helper->GetPosition());
      m_helper->UpdateBackgroundParams(m_params, m_tex);
    }

    return Handle::Update(screen);
  }

private:
  RenderingHelperPtr m_helper;
  ref_ptr<dp::TextureManager> m_tex;
};
}  // namespace handle

BackgroundRenderer::BackgroundRenderer(RenderingHelperPtr helper) : m_helper(std::move(helper)) {}

void BackgroundRenderer::Draw(ref_ptr<dp::GraphicsContext> context, ShapeControl & control,
                              ref_ptr<dp::TextureManager> tex) const
{
  auto state = df::CreateRenderState(gpu::Program::GuiRoundRect, df::DepthLayer::GuiLayer);
  state.SetDepthTestEnabled(false);

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, vertex::BackgroundVertex::GetBindingInfo(), make_ref(vertex::createVertexData().data()));
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<handle::BackgroundHandle>(m_helper, std::move(tex));

  dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
  batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
  dp::SessionGuard guard(context, batcher, std::bind(&ShapeControl::AddShape, &control, _1, _2));
  batcher.InsertTriangleStrip(context, state, make_ref(&provider), std::move(handle));
}
}  // namespace gui::speed_limit::renderer
