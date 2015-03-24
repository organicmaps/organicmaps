#include "button.hpp"
#include "country_status.hpp"
#include "country_status_helper.hpp"
#include "gui_text.hpp"
#include "drape_gui.hpp"

#include "../drape/batcher.hpp"
#include "../drape/glsl_func.hpp"

#include "../std/bind.hpp"

namespace gui
{
namespace
{
class CountryStatusHandle : public Handle
{
  typedef Handle TBase;

public:
  CountryStatusHandle(int state, dp::Anchor anchor, m2::PointF const & size)
      : Handle(anchor, m2::PointF::Zero(), size), m_state(state)
  {
  }

  void Update(ScreenBase const & screen) override
  {
    SetIsVisible(DrapeGui::GetCountryStatusHelper().IsVisibleForState(m_state));
    TBase::Update(screen);
  }

private:
  int m_state;
};

class CountryProgressHandle : public MutableLabelHandle
{
  using TBase = MutableLabelHandle;

public:
  CountryProgressHandle(dp::Anchor anchor, int state)
      : MutableLabelHandle(anchor, m2::PointF::Zero()), m_state(state)
  {
  }

  void Update(ScreenBase const & screen) override
  {
    CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
    SetIsVisible(helper.IsVisibleForState(m_state));
    if (IsVisible())
    {
      string v = helper.GetProgressValue();
      if (m_value != v)
      {
        m_value = move(v);
        m_contentDirty = true;
      }
      else
        m_contentDirty = false;
    }

    TBase::Update(screen);
  }

protected:
  bool IsContentDirty() const override { return m_contentDirty; }

  string GetContent() const override { return m_value; }

private:
  int m_state;
  bool m_contentDirty;
  string m_value;
};

dp::TransferPointer<dp::OverlayHandle> CreateHandle(int state, dp::Anchor anchor,
                                                    m2::PointF const & size)
{
  return dp::MovePointer<dp::OverlayHandle>(new CountryStatusHandle(state, anchor, size));
}

void DrawLabelControl(string const & text, dp::Anchor anchor, dp::Batcher::TFlushFn const & flushFn,
                      dp::RefPointer<dp::TextureManager> mng, int state)
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText(text, "\n", anchor, dp::FontDecl(dp::Color::Black(), 18), mng,
                               result);
  size_t vertexCount = result.m_buffer.size();
  ASSERT(vertexCount % 4 == 0, ());
  size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

  dp::AttributeProvider provider(1 /*stream count*/, vertexCount);
  provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                      dp::MakeStackRefPointer<void>(result.m_buffer.data()));

  dp::Batcher batcher(indexCount, vertexCount);
  dp::SessionGuard guard(batcher, flushFn);
  m2::PointF size(result.m_boundRect.SizeX(), result.m_boundRect.SizeY());
  dp::MasterPointer<dp::OverlayHandle> handle(new CountryStatusHandle(state, anchor, size));
  batcher.InsertListOfStrip(result.m_state, dp::MakeStackRefPointer(&provider), handle.Move(),
                            dp::Batcher::VertexPerQuad);
}

void DrawProgressControl(dp::Anchor anchor, dp::Batcher::TFlushFn const & flushFn,
                         dp::RefPointer<dp::TextureManager> mng, int state)
{
  MutableLabelDrawer::Params params;
  CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
  helper.GetProgressInfo(params.m_alphabet, params.m_maxLength);

  params.m_anchor = anchor;
  params.m_pivot = m2::PointF::Zero();
  params.m_font = dp::FontDecl(dp::Color::Black(), 18);
  params.m_handleCreator = [state](dp::Anchor anchor, m2::PointF const & /*pivot*/)
  {
    return dp::MovePointer<MutableLabelHandle>(new CountryProgressHandle(anchor, state));
  };

  MutableLabelDrawer::Draw(params, mng, flushFn);
}
}

dp::TransferPointer<ShapeRenderer> CountryStatus::Draw(dp::RefPointer<dp::TextureManager> tex) const
{
  dp::MasterPointer<ShapeRenderer> renderer(new ShapeRenderer());
  dp::Batcher::TFlushFn flushFn = bind(&ShapeRenderer::AddShape, renderer.GetRaw(), _1, _2);

  CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
  int const state = helper.GetState();

  for (size_t i = 0; i < helper.GetComponentCount(); ++i)
  {
    CountryStatusHelper::Control const & control = helper.GetControl(i);
    switch (control.m_type)
    {
      case CountryStatusHelper::Button:
      {
        Button::THandleCreator handleCreator = bind(&CreateHandle, state, _1, _2);
        ShapeControl shapeControl;
        Button::Params params;
        params.m_anchor = m_position.m_anchor;
        params.m_label = control.m_label;
        params.m_labelFont = dp::FontDecl(dp::Color::White(), 16);
        params.m_minWidth = 300;
        params.m_maxWidth = 600;
        params.m_margin = 5.0f * DrapeGui::Instance().GetScaleFactor();
        params.m_bodyHandleCreator = handleCreator;
        params.m_labelHandleCreator = handleCreator;

        Button button(params);
        button.Draw(shapeControl, tex);
        renderer->AddShapeControl(move(shapeControl));
      }
      break;
      case CountryStatusHelper::Label:
        DrawLabelControl(control.m_label, m_position.m_anchor, flushFn, tex, state);
        break;
      case CountryStatusHelper::Progress:
        DrawProgressControl(m_position.m_anchor, flushFn, tex, state);
        break;
      default:
        ASSERT(false, ());
        break;
    }
  }

  buffer_vector<float, 4> heights;
  float summaryHeight = 0.0f;

  ArrangeShapes(renderer.GetRefPointer(), [&heights, &summaryHeight](ShapeControl & shape)
  {
    float height = 0.0f;
    for (ShapeControl::ShapeInfo & info : shape.m_shapesInfo)
      height = max(height, info.m_handle->GetSize().y);

    heights.push_back(height);
    summaryHeight += height;
  });

  float const controlMargin = helper.GetControlMargin();
  summaryHeight += controlMargin * (heights.size() - 1);
  float halfHeight = summaryHeight / 2.0f;
  glsl::vec2 pen(m_position.m_pixelPivot.x, m_position.m_pixelPivot.y - halfHeight);
  size_t controlIndex = 0;

  ArrangeShapes(renderer.GetRefPointer(), [&](ShapeControl & shape)
  {
    float const h = heights[controlIndex];
    float const halfH = h / 2.0f;
    ++controlIndex;

    for (ShapeControl::ShapeInfo & info : shape.m_shapesInfo)
      info.m_handle->SetPivot(pen + glsl::vec2(0.0f, halfH));

    pen.y += (h + controlMargin);
  });

  return renderer.Move();
}

}  // namespace gui
