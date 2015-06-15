#include "copyright_label.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"
#include "ruler_helper.hpp"

#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/animation/value_mapping.hpp"

#include "base/timer.hpp"

#include "std/bind.hpp"

namespace gui
{

namespace
{
  double const COPYRIGHT_VISIBLE_TIME = 3.0f;
  double const COPYRIGHT_HIDE_DURATION = 0.25f;

  class CopyrightHandle : public Handle
  {
    using TBase = Handle;
  public:
    CopyrightHandle(dp::Anchor anchor, m2::PointF const & pivot, m2::PointF const & size)
      : TBase(anchor, pivot, size)
    {
      SetIsVisible(true);
    }

    void Update(ScreenBase const & screen) override
    {
      if (!IsVisible())
        return;

      TBase::Update(screen);

      if (m_animation == nullptr)
        m_animation.reset(new df::OpacityAnimation(COPYRIGHT_HIDE_DURATION, COPYRIGHT_VISIBLE_TIME, 1.0f, 0.0f));
      else if (m_animation->IsFinished())
      {
        DrapeGui::Instance().DeactivateCopyright();
        SetIsVisible(false);
      }

      m_uniforms.SetFloatValue("u_opacity", m_animation->GetOpacity());
    }

  private:
    drape_ptr<df::OpacityAnimation> m_animation;
  };
}

CopyrightLabel::CopyrightLabel(Position const & position)
  : TBase(position)
{
}

drape_ptr<ShapeRenderer> CopyrightLabel::Draw(m2::PointF & size, ref_ptr<dp::TextureManager> tex) const
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText("Map data © OpenStreetMap", "", m_position.m_anchor,
                               DrapeGui::GetGuiTextFont(), tex, result);

  dp::AttributeProvider provider(1 /*stream count*/, result.m_buffer.size());
  provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                      make_ref(result.m_buffer.data()));

  size_t vertexCount = result.m_buffer.size();
  ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
  size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

  size = m2::PointF(result.m_boundRect.SizeX(), result.m_boundRect.SizeY());
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<CopyrightHandle>(m_position.m_anchor, m_position.m_pixelPivot, size);

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  dp::Batcher batcher(indexCount, vertexCount);
  dp::SessionGuard guard(batcher, bind(&ShapeRenderer::AddShape, renderer.get(), _1, _2));
  batcher.InsertListOfStrip(result.m_state, make_ref(&provider),
                            move(handle), dp::Batcher::VertexPerQuad);

  return renderer;
}

}
