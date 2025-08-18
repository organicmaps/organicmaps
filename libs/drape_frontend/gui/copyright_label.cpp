#include "copyright_label.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"

#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/batcher_bucket.hpp"

#include <functional>
#include <utility>

namespace gui
{
namespace
{
constexpr double kCopyrightVisibleTime = 10.0f;
constexpr double kCopyrightHideTime = 0.25f;

class CopyrightHandle : public StaticLabelHandle
{
  using TBase = StaticLabelHandle;

public:
  CopyrightHandle(uint32_t id, ref_ptr<dp::TextureManager> textureManager, dp::Anchor anchor, m2::PointF const & pivot,
                  dp::TGlyphs && glyphs)
    : TBase(id, textureManager, anchor, pivot, std::move(glyphs))
  {
    SetIsVisible(true);
  }

  bool Update(ScreenBase const & screen) override
  {
    if (!IsVisible())
      return false;

    if (!TBase::Update(screen))
      return false;

    if (!DrapeGui::Instance().IsCopyrightActive())
    {
      SetIsVisible(false);
      return false;
    }

    if (m_animation == nullptr)
    {
      m_animation = make_unique_dp<df::OpacityAnimation>(kCopyrightHideTime, kCopyrightVisibleTime, 1.0f, 0.0f);
    }
    else if (m_animation->IsFinished())
    {
      DrapeGui::Instance().DeactivateCopyright();
      SetIsVisible(false);
    }

    m_params.m_opacity = static_cast<float>(m_animation->GetOpacity());
    return true;
  }

private:
  drape_ptr<df::OpacityAnimation> m_animation;
};
}  // namespace

CopyrightLabel::CopyrightLabel(Position const & position) : TBase(position) {}

drape_ptr<ShapeRenderer> CopyrightLabel::Draw(ref_ptr<dp::GraphicsContext> context,
                                              ref_ptr<dp::TextureManager> tex) const
{
  StaticLabel::LabelResult result;
  auto glyphs = StaticLabel::CacheStaticText("Map data © OpenStreetMap", "", m_position.m_anchor,
                                             DrapeGui::GetGuiTextFont(), tex, result);

  dp::AttributeProvider provider(1 /*stream count*/, static_cast<uint32_t>(result.m_buffer.size()));
  provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(), make_ref(result.m_buffer.data()));

  auto const vertexCount = static_cast<uint32_t>(result.m_buffer.size());
  ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
  auto const indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<CopyrightHandle>(GuiHandleCopyright, tex, m_position.m_anchor,
                                                                        m_position.m_pixelPivot, std::move(glyphs));

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  dp::Batcher batcher(indexCount, vertexCount);
  batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
  using namespace std::placeholders;
  dp::SessionGuard guard(context, batcher, std::bind(&ShapeRenderer::AddShape, renderer.get(), _1, _2));
  batcher.InsertListOfStrip(context, result.m_state, make_ref(&provider), std::move(handle),
                            dp::Batcher::VertexPerQuad);

  return renderer;
}
}  // namespace gui
