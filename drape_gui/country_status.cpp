#include "button.hpp"
#include "country_status.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"

#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

namespace gui
{
namespace
{

class CountryStatusButtonHandle : public ButtonHandle
{
  using TBase = ButtonHandle;

public:
  CountryStatusButtonHandle(CountryStatusHelper::ECountryState const state,
                            Shape::TTapHandler const & tapHandler,
                            dp::Anchor anchor, m2::PointF const & size)
    : TBase(anchor, size)
    , m_state(state)
    , m_tapHandler(tapHandler)
  {}

  void OnTap() override
  {
    if (m_tapHandler != nullptr)
      m_tapHandler();
  }

  void Update(ScreenBase const & screen) override
  {
    SetIsVisible(DrapeGui::GetCountryStatusHelper().IsVisibleForState(m_state));
    TBase::Update(screen);
  }

private:
  CountryStatusHelper::ECountryState m_state;
  Shape::TTapHandler m_tapHandler;
};

class CountryStatusLabelHandle : public Handle
{
  using TBase = Handle;

public:
  CountryStatusLabelHandle(CountryStatusHelper::ECountryState const state,
                           dp::Anchor anchor, m2::PointF const & size)
    : TBase(anchor, m2::PointF::Zero(), size)
    , m_state(state)
  {}

  void Update(ScreenBase const & screen) override
  {
    SetIsVisible(DrapeGui::GetCountryStatusHelper().IsVisibleForState(m_state));
    TBase::Update(screen);
  }

private:
  CountryStatusHelper::ECountryState m_state;
};

class CountryProgressHandle : public MutableLabelHandle
{
  using TBase = MutableLabelHandle;

public:
  CountryProgressHandle(dp::Anchor anchor, CountryStatusHelper::ECountryState const state)
    : TBase(anchor, m2::PointF::Zero()), m_state(state)
  {}

  void Update(ScreenBase const & screen) override
  {
    CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
    SetIsVisible(helper.IsVisibleForState(m_state));
    if (IsVisible())
      SetContent(helper.GetProgressValue());

    TBase::Update(screen);
  }

private:
  CountryStatusHelper::ECountryState m_state;
};

drape_ptr<dp::OverlayHandle> CreateButtonHandle(CountryStatusHelper::ECountryState const state,
                                                Shape::TTapHandler const & tapHandler,
                                                dp::Anchor anchor, m2::PointF const & size)
{
  return make_unique_dp<CountryStatusButtonHandle>(state, tapHandler, anchor, size);
}

drape_ptr<dp::OverlayHandle> CreateLabelHandle(CountryStatusHelper::ECountryState const state,
                                               dp::Anchor anchor,
                                               m2::PointF const & size)
{
  return make_unique_dp<CountryStatusLabelHandle>(state, anchor, size);
}

void DrawLabelControl(string const & text, dp::Anchor anchor, dp::Batcher::TFlushFn const & flushFn,
                      ref_ptr<dp::TextureManager> mng, CountryStatusHelper::ECountryState state)
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText(text, "\n", anchor, dp::FontDecl(dp::Color::Black(), 18), mng,
                               result);
  size_t vertexCount = result.m_buffer.size();
  ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
  size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

  dp::AttributeProvider provider(1 /*stream count*/, vertexCount);
  provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                      make_ref(result.m_buffer.data()));

  dp::Batcher batcher(indexCount, vertexCount);
  dp::SessionGuard guard(batcher, flushFn);
  m2::PointF size(result.m_boundRect.SizeX(), result.m_boundRect.SizeY());
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<CountryStatusLabelHandle>(state, anchor, size);
  batcher.InsertListOfStrip(result.m_state, make_ref(&provider), move(handle),
                            dp::Batcher::VertexPerQuad);
}

void DrawProgressControl(dp::Anchor anchor, dp::Batcher::TFlushFn const & flushFn,
                         ref_ptr<dp::TextureManager> mng, CountryStatusHelper::ECountryState state)
{
  MutableLabelDrawer::Params params;
  CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
  helper.GetProgressInfo(params.m_alphabet, params.m_maxLength);

  params.m_anchor = anchor;
  params.m_pivot = m2::PointF::Zero();
  params.m_font = dp::FontDecl(dp::Color::Black(), 18);
  params.m_handleCreator = [state](dp::Anchor anchor, m2::PointF const & /*pivot*/)
  {
    return make_unique_dp<CountryProgressHandle>(anchor, state);
  };

  MutableLabelDrawer::Draw(params, mng, flushFn);
}

}

drape_ptr<ShapeRenderer> CountryStatus::Draw(ref_ptr<dp::TextureManager> tex,
                                             TButtonHandlers const & buttonHandlers) const
{
  CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
  if (helper.GetComponentCount() == 0)
    return nullptr;

  CountryStatusHelper::ECountryState const state = helper.GetState();
  ASSERT(state != CountryStatusHelper::COUNTRY_STATE_LOADED, ());

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  dp::Batcher::TFlushFn flushFn = bind(&ShapeRenderer::AddShape, renderer.get(), _1, _2);

  for (size_t i = 0; i < helper.GetComponentCount(); ++i)
  {
    CountryStatusHelper::Control const & control = helper.GetControl(i);
    switch (control.m_type)
    {
    case CountryStatusHelper::CONTROL_TYPE_BUTTON:
      {
        TButtonHandlers::const_iterator buttonHandlerIt = buttonHandlers.find(control.m_buttonType);
        Shape::TTapHandler buttonHandler = (buttonHandlerIt != buttonHandlers.end() ? buttonHandlerIt->second : nullptr);
        Button::THandleCreator buttonHandleCreator = bind(&CreateButtonHandle, state, buttonHandler, _1, _2);
        Button::THandleCreator labelHandleCreator = bind(&CreateLabelHandle, state, _1, _2);

        ShapeControl shapeControl;
        Button::Params params;
        params.m_anchor = m_position.m_anchor;
        params.m_label = control.m_label;
        params.m_labelFont = dp::FontDecl(dp::Color::White(), 16);
        params.m_minWidth = 300;
        params.m_maxWidth = 600;
        params.m_margin = 5.0f * DrapeGui::Instance().GetScaleFactor();
        params.m_facet = 8.0f * DrapeGui::Instance().GetScaleFactor();
        params.m_bodyHandleCreator = buttonHandleCreator;
        params.m_labelHandleCreator = labelHandleCreator;

        Button::Draw(params, shapeControl, tex);
        renderer->AddShapeControl(move(shapeControl));
      }
      break;
    case CountryStatusHelper::CONTROL_TYPE_LABEL:
      DrawLabelControl(control.m_label, m_position.m_anchor, flushFn, tex, state);
      break;
    case CountryStatusHelper::CONTROL_TYPE_PROGRESS:
      DrawProgressControl(m_position.m_anchor, flushFn, tex, state);
      break;
    default:
      ASSERT(false, ());
      break;
    }
  }

  buffer_vector<float, 4> heights;
  float totalHeight = 0.0f;

  ArrangeShapes(make_ref(renderer), [&heights, &totalHeight](ShapeControl & shape)
  {
    float height = 0.0f;
    for (ShapeControl::ShapeInfo & info : shape.m_shapesInfo)
      height = max(height, info.m_handle->GetSize().y);

    heights.push_back(height);
    totalHeight += height;
  });

  ASSERT(!heights.empty(), ());

  float const controlMargin = helper.GetControlMargin();
  totalHeight += controlMargin * (heights.size() - 1);
  float halfHeight = totalHeight * 0.5f;
  glsl::vec2 pen(m_position.m_pixelPivot.x, m_position.m_pixelPivot.y - halfHeight);
  size_t controlIndex = 0;

  ArrangeShapes(make_ref(renderer), [&](ShapeControl & shape)
  {
    float const h = heights[controlIndex];
    float const halfH = h * 0.5f;
    ++controlIndex;

    for (ShapeControl::ShapeInfo & info : shape.m_shapesInfo)
      info.m_handle->SetPivot(pen + glsl::vec2(0.0f, halfH));

    pen.y += (h + controlMargin);
  });

  return renderer;
}

}  // namespace gui
