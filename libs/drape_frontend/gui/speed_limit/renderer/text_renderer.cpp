#include "text_renderer.hpp"

#include "base/type_tag.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/gui/speed_limit/speed_limit.hpp"

#include "drape/glsl_types.hpp"

#include <functional>
#include <utility>

using namespace std::placeholders;

namespace gui::speed_limit::renderer
{
namespace handle
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
    static int v = 0;
    SpeedLimit const & helper = DrapeGui::GetSpeedLimit();
    SetIsVisible(helper.IsEnabled());
    if (IsVisible())
    {
      m_pivot = glsl::ToVec2(helper.GetPosition());
      SetContent(std::to_string(v / 10));
      v++;
      v = v % 10000;
    }

    return TBase::Update(screen);
  }
};
}  // namespace handle

TextRenderer::TextRenderer(RenderingHelperPtr helper) : m_helper(std::move(helper)) {}

void TextRenderer::Draw(ref_ptr<dp::GraphicsContext> context, ShapeControl & control,
                        ref_ptr<dp::TextureManager> tex) const
{
  MutableLabelDrawer::Params params;
  params.m_anchor = dp::Center;
  params.m_alphabet = "0123456789";
  params.m_maxLength = 3;
  params.m_font = DrapeGui::GetGuiTextFont();
  params.m_font.m_color = DrapeGui::GetSpeedLimit().GetConfig().textColor;
  params.m_font.m_size = 360;
  params.m_pivot = m_helper->GetPosition();
  params.m_handleCreator = [tex](dp::Anchor, m2::PointF const & pivot)
  { return make_unique_dp<handle::TextHandle>(pivot, tex); };

  MutableLabelDrawer::Draw(context, params, tex, std::bind(&ShapeControl::AddShape, &control, _1, _2));
}
}  // namespace gui::speed_limit::renderer
