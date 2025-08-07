#include "speed_limit_renderer.hpp"

#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/gui/speed_limit/speed_limit.hpp"

#include "drape/glsl_types.hpp"

#include <functional>
#include <utility>

using namespace std::placeholders;

namespace gui::speed_limit::renderer
{
namespace
{
class TextHandle : public MutableLabelHandle
{
  using TBase = MutableLabelHandle;

public:
  TextHandle(m2::PointF const & pivot, ref_ptr<dp::TextureManager> const & textures)
    : TBase(base::TypeTag<TextHandle>::id<std::uint32_t>(), dp::Center, pivot)
  {
    SetTextureManager(textures);
  }

private:
  bool Update(ScreenBase const & screen) override
  {
    SpeedLimit const & helper = DrapeGui::GetSpeedLimitHelper();
    SetIsVisible(helper.IsEnabled() && helper.IsSpeedLimitAvailable());
    if (IsVisible())
    {
      m_pivot = glsl::ToVec2(helper.GetPosition());
      SetContent(helper.GetSpeedLimit());
    }

    return TBase::Update(screen);
  }
};
}  // namespace

SpeedLimitRenderer::SpeedLimitRenderer(Position const & position) : Shape(position)
{
  m_background.SetPosition(position);
  m_background.SetRadius(DrapeGui::GetSpeedLimitHelper().GetSize());
}

drape_ptr<ShapeRenderer> SpeedLimitRenderer::Draw(ref_ptr<dp::GraphicsContext> context,
                                                  ref_ptr<dp::TextureManager> tex) const
{
  LOG(LWARNING, ("Draw SpeedLimit"));
  ShapeControl control;
  m_background.Draw(context, control);
  DrawText(context, control, tex);

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  renderer->AddShapeControl(std::move(control));
  return renderer;
}

void SpeedLimitRenderer::DrawText(ref_ptr<dp::GraphicsContext> context, ShapeControl & control,
                                  ref_ptr<dp::TextureManager> tex) const
{
  ASSERT_EQUAL(m_position.m_anchor, dp::Center, ());

  MutableLabelDrawer::Params params;
  params.m_anchor = m_position.m_anchor;
  params.m_alphabet = "0123456789";
  params.m_maxLength = 3;
  params.m_font = DrapeGui::GetGuiTextFont();
  params.m_font.m_color = DrapeGui::GetSpeedLimitHelper().GetConfig().textColor;
  params.m_font.m_size = 24;
  params.m_pivot = m_position.m_pixelPivot;
  params.m_handleCreator = [tex](dp::Anchor, m2::PointF const & pivot)
  { return make_unique_dp<TextHandle>(pivot, tex); };

  MutableLabelDrawer::Draw(context, params, tex, std::bind(&ShapeControl::AddShape, &control, _1, _2));
}
}  // namespace gui::speed_limit::renderer
