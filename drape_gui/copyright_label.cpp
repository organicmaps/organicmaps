#include "copyright_label.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"
#include "ruler_helper.hpp"

#include "../base/timer.hpp"

#include "../std/bind.hpp"

namespace gui
{

namespace
{
  float const CopyrightVisibleTime = 3.0f;

  class CopyrightHandle : public Handle
  {
    using TBase = Handle;
  public:
    CopyrightHandle(dp::Anchor anchor, m2::PointF const & pivot, m2::PointF const & size)
      : TBase(anchor, pivot, size)
      , m_timer(false)
      , m_firstRender(true)
    {
      SetIsVisible(true);
    }

    void Update(ScreenBase const & screen) override
    {
      if (!IsVisible())
        return;

      if (m_firstRender == true)
      {
        m_firstRender = false;
        m_timer.Reset();
      }
      else if (m_timer.ElapsedSeconds() > CopyrightVisibleTime)
      {
        DrapeGui::Instance().DeactivateCopyright();
        SetIsVisible(false);
      }

      TBase::Update(screen);
    }

  private:
    my::Timer m_timer;
    bool m_firstRender;
  };
}

CopyrightLabel::CopyrightLabel(Position const & position)
  : TBase(position)
{
}

dp::TransferPointer<ShapeRenderer> CopyrightLabel::Draw(dp::RefPointer<dp::TextureManager> tex) const
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText("Map data © OpenStreetMap", "", m_position.m_anchor,
                               DrapeGui::GetGuiTextFont(), tex, result);

  dp::AttributeProvider provider(1 /*stream count*/, result.m_buffer.size());
  provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                      dp::StackVoidRef(result.m_buffer.data()));

  size_t vertexCount = result.m_buffer.size();
  ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
  size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

  m2::PointF size(result.m_boundRect.SizeX(), result.m_boundRect.SizeY());
  dp::MasterPointer<dp::OverlayHandle> handle(new CopyrightHandle(m_position.m_anchor,
                                                                  m_position.m_pixelPivot,
                                                                  size));

  dp::MasterPointer<ShapeRenderer> renderer(new ShapeRenderer());
  dp::Batcher batcher(indexCount, vertexCount);
  dp::SessionGuard guard(batcher, bind(&ShapeRenderer::AddShape, renderer.GetRaw(), _1, _2));
  batcher.InsertListOfStrip(result.m_state, dp::MakeStackRefPointer(&provider),
                            handle.Move(), dp::Batcher::VertexPerQuad);

  return renderer.Move();
}

}
