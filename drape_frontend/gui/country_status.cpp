#include "button.hpp"
#include "country_status.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"

#include "drape_frontend/visual_params.hpp"

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
                            dp::Anchor anchor, m2::PointF const & size,
                            dp::Color const & color, dp::Color const & pressedColor)
    : TBase(anchor, size, color, pressedColor)
    , m_state(state)
    , m_tapHandler(tapHandler)
  {}

  void OnTap() override
  {
    if (m_tapHandler != nullptr)
      m_tapHandler();
  }

  bool Update(ScreenBase const & screen) override
  {
    SetIsVisible(DrapeGui::GetCountryStatusHelper().IsVisibleForState(m_state));
    return TBase::Update(screen);
  }

private:
  CountryStatusHelper::ECountryState m_state;
  Shape::TTapHandler m_tapHandler;
};

class CountryStatusLabelHandle : public StaticLabelHandle
{
  using TBase = StaticLabelHandle;

public:
  CountryStatusLabelHandle(CountryStatusHelper::ECountryState const state,
                           ref_ptr<dp::TextureManager> textureManager,
                           dp::Anchor anchor, m2::PointF const & size,
                           TAlphabet const & alphabet)
    : TBase(textureManager, anchor, m2::PointF::Zero(), size, alphabet)
    , m_state(state)
  {}

  bool Update(ScreenBase const & screen) override
  {
    SetIsVisible(DrapeGui::GetCountryStatusHelper().IsVisibleForState(m_state));
    return TBase::Update(screen);
  }

private:
  CountryStatusHelper::ECountryState m_state;
};

class CountryProgressHandle : public MutableLabelHandle
{
  using TBase = MutableLabelHandle;

public:
  CountryProgressHandle(dp::Anchor anchor, CountryStatusHelper::ECountryState const state,
                        ref_ptr<dp::TextureManager> textures)
    : TBase(anchor, m2::PointF::Zero(), textures), m_state(state)
  {}

  bool Update(ScreenBase const & screen) override
  {
    CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
    SetIsVisible(helper.IsVisibleForState(m_state));
    if (IsVisible())
      SetContent(helper.GetProgressValue());

    return TBase::Update(screen);
  }

private:
  CountryStatusHelper::ECountryState m_state;
};

drape_ptr<dp::OverlayHandle> CreateButtonHandle(CountryStatusHelper::ECountryState const state,
                                                Shape::TTapHandler const & tapHandler,
                                                dp::Color const & color, dp::Color const & pressedColor,
                                                dp::Anchor anchor, m2::PointF const & size)
{
  return make_unique_dp<CountryStatusButtonHandle>(state, tapHandler, anchor, size, color, pressedColor);
}

drape_ptr<dp::OverlayHandle> CreateLabelHandle(CountryStatusHelper::ECountryState const state,
                                               ref_ptr<dp::TextureManager> textureManager,
                                               dp::Anchor anchor, m2::PointF const & size,
                                               TAlphabet const & alphabet)
{
  return make_unique_dp<CountryStatusLabelHandle>(state, textureManager, anchor, size, alphabet);
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
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<CountryStatusLabelHandle>(state, mng, anchor, size, result.m_alphabet);
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
  params.m_handleCreator = [state, mng](dp::Anchor anchor, m2::PointF const & /*pivot*/)
  {
    return make_unique_dp<CountryProgressHandle>(anchor, state, mng);
  };

  MutableLabelDrawer::Draw(params, mng, flushFn);
}

void ForEachComponent(CountryStatusHelper & helper, CountryStatusHelper::EControlType type,
                      function<void(CountryStatusHelper::Control const &)> const & callback)
{
  for (size_t i = 0; i < helper.GetComponentCount(); ++i)
  {
    CountryStatusHelper::Control const & control = helper.GetControl(i);
    if (callback != nullptr && control.m_type == type)
      callback(control);
  }
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

  // Precache progress symbols.
  {
    CountryStatusHelper & helper = DrapeGui::GetCountryStatusHelper();
    string alphabet;
    size_t maxLength;
    helper.GetProgressInfo(alphabet, maxLength);
    dp::TextureManager::TGlyphsBuffer buffer;
    tex->GetGlyphRegions(strings::MakeUniString(alphabet), buffer);
  }

  // Create labels.
  ForEachComponent(helper, CountryStatusHelper::CONTROL_TYPE_LABEL,
                   [this, &tex, &flushFn, &state](CountryStatusHelper::Control const & control)
  {
    DrawLabelControl(control.m_label, m_position.m_anchor, flushFn, tex, state);
  });

  // Preprocess buttons.
  vector<pair<Button::Params, StaticLabel::LabelResult>> buttons;
  float const kMinButtonWidth = 400;
  float maxButtonWidth = kMinButtonWidth;
  buttons.reserve(2);
  ForEachComponent(helper, CountryStatusHelper::CONTROL_TYPE_BUTTON,
                   [this, &buttons, &state, &tex, &buttonHandlers,
                   &maxButtonWidth](CountryStatusHelper::Control const & control)
  {
    float const visualScale = df::VisualParams::Instance().GetVisualScale();

    Button::Params params;
    params.m_anchor = m_position.m_anchor;
    params.m_label = control.m_label;
    params.m_labelFont = dp::FontDecl(dp::Color::White(), 18);
    params.m_margin = 5.0f * visualScale;
    params.m_facet = 8.0f * visualScale;

    auto color = dp::Color(0, 0, 0, 0.44 * 255);
    auto pressedColor = dp::Color(0, 0, 0, 0.72 * 255);
    if (control.m_buttonType == CountryStatusHelper::BUTTON_TYPE_MAP_ROUTING)
    {
      color = dp::Color(32, 152, 82, 255);
      pressedColor = dp::Color(24, 128, 68, 255);
    }

    auto const buttonHandlerIt = buttonHandlers.find(control.m_buttonType);
    Shape::TTapHandler buttonHandler = (buttonHandlerIt != buttonHandlers.end() ? buttonHandlerIt->second : nullptr);
    params.m_bodyHandleCreator = bind(&CreateButtonHandle, state, buttonHandler, color, pressedColor, _1, _2);
    params.m_labelHandleCreator = bind(&CreateLabelHandle, state, tex, _1, _2, _3);

    auto label = Button::PreprocessLabel(params, tex);
    float const buttonWidth = label.m_boundRect.SizeX();
    if (buttonWidth > maxButtonWidth)
      maxButtonWidth = buttonWidth;

    buttons.emplace_back(make_pair(move(params), move(label)));
  });

  // Create buttons.
  for (size_t i = 0; i < buttons.size(); i++)
  {
    buttons[i].first.m_width = maxButtonWidth;
    ShapeControl shapeControl;
    Button::Draw(buttons[i].first, shapeControl, buttons[i].second);
    renderer->AddShapeControl(move(shapeControl));
  }

  // Create progress bars.
  ForEachComponent(helper, CountryStatusHelper::CONTROL_TYPE_PROGRESS,
                   [this, &tex, &flushFn, &state](CountryStatusHelper::Control const &)
  {
    DrawProgressControl(m_position.m_anchor, flushFn, tex, state);
  });

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
