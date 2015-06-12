#include "compass.hpp"
#include "copyright_label.hpp"
#include "country_status.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"
#include "layer_render.hpp"
#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "drape/batcher.hpp"
#include "drape/render_bucket.hpp"

#include "base/stl_add.hpp"

#include "std/bind.hpp"

namespace gui
{

LayerRenderer::~LayerRenderer()
{
  DestroyRenderers();
}

void LayerRenderer::Build(ref_ptr<dp::GpuProgramManager> mng)
{
  for (TRenderers::value_type & r : m_renderers)
    r.second->Build(mng);
}

void LayerRenderer::Render(ref_ptr<dp::GpuProgramManager> mng, ScreenBase const & screen)
{
  DrapeGui::Instance().GetRulerHelper().Update(screen);
  for (TRenderers::value_type & r : m_renderers)
    r.second->Render(screen, mng);
}

void LayerRenderer::Merge(ref_ptr<LayerRenderer> other)
{
  for (TRenderers::value_type & r : other->m_renderers)
  {
    TRenderers::iterator it = m_renderers.find(r.first);
    if (it != m_renderers.end())
      it->second = move(r.second);
    else
      m_renderers.insert(make_pair(r.first, move(r.second)));
  }

  other->m_renderers.clear();
}

void LayerRenderer::SetLayout(TWidgetsLayoutInfo const & info)
{
  for (auto node : info)
  {
    auto renderer = m_renderers.find(node.first);
    if (renderer != m_renderers.end())
      renderer->second->SetPivot(node.second);
  }
}

void LayerRenderer::DestroyRenderers()
{
  m_renderers.clear();
}

void LayerRenderer::AddShapeRenderer(EWidget widget, drape_ptr<ShapeRenderer> && shape)
{
  if (shape == nullptr)
    return;

  VERIFY(m_renderers.insert(make_pair(widget, move(shape))).second, ());
}

bool LayerRenderer::OnTouchDown(m2::RectD const & touchArea)
{
  for (TRenderers::value_type & r : m_renderers)
  {
    m_activeOverlay = r.second->ProcessTapEvent(touchArea);
    if (m_activeOverlay != nullptr)
    {
      m_activeOverlay->OnTapBegin();
      return true;
    }
  }

  return false;
}

void LayerRenderer::OnTouchUp(m2::RectD const & touchArea)
{
  if (m_activeOverlay != nullptr)
  {
    if (m_activeOverlay->IsTapped(touchArea))
      m_activeOverlay->OnTap();

    m_activeOverlay->OnTapEnd();
    m_activeOverlay = nullptr;
  }
}

void LayerRenderer::OnTouchCancel(m2::RectD const & touchArea)
{
  UNUSED_VALUE(touchArea);
  if (m_activeOverlay != nullptr)
  {
    m_activeOverlay->OnTapEnd();
    m_activeOverlay = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////////

namespace
{

class ScaleLabelHandle : public MutableLabelHandle
{
  using TBase = MutableLabelHandle;
public:
  ScaleLabelHandle()
    : TBase(dp::LeftBottom, m2::PointF::Zero())
    , m_scale(0)
  {
    SetIsVisible(true);
  }

  void Update(ScreenBase const & screen) override
  {
    int newScale = DrapeGui::Instance().GetGeneralization(screen);
    if (m_scale != newScale)
    {
      m_scale = newScale;
      SetContent("Scale : " + strings::to_string(m_scale));
    }

    float vs = DrapeGui::Instance().GetScaleFactor();
    m2::PointF offset(10.0f * vs, 30.0f * vs);

    SetPivot(glsl::ToVec2(m2::PointF(screen.PixelRect().LeftBottom()) + offset));
    TBase::Update(screen);
  }

private:
  int m_scale;
};

void RegisterButtonHandler(CountryStatus::TButtonHandlers & handlers,
                           CountryStatusHelper::EButtonType buttonType)
{
  handlers[buttonType] = bind(&DrapeGui::CallOnButtonPressedHandler,
                              &DrapeGui::Instance(), buttonType);
}

} // namespace

drape_ptr<LayerRenderer> LayerCacher::RecacheWidgets(TWidgetsInitInfo const & initInfo,
                                                     TWidgetsSizeInfo & sizeInfo,
                                                     ref_ptr<dp::TextureManager> textures)
{
  using TCacheShape = function<m2::PointF (Position anchor, ref_ptr<LayerRenderer> renderer, ref_ptr<dp::TextureManager> textures)>;
  static map<EWidget, TCacheShape> cacheFunctions
  {
    make_pair(WIDGET_COMPASS, bind(&LayerCacher::CacheCompass, this, _1, _2, _3)),
    make_pair(WIDGET_RULER, bind(&LayerCacher::CacheRuler, this, _1, _2, _3)),
    make_pair(WIDGET_COPYRIGHT, bind(&LayerCacher::CacheCopyright, this, _1, _2, _3)),
    make_pair(WIDGET_SCALE_LABLE, bind(&LayerCacher::CacheScaleLabel, this, _1, _2, _3))
  };

  drape_ptr<LayerRenderer> renderer = make_unique_dp<LayerRenderer>();
  for (auto node : initInfo)
  {
    auto cacheFunction = cacheFunctions.find(node.first);
    if (cacheFunction != cacheFunctions.end())
      sizeInfo[node.first] = cacheFunction->second(node.second, make_ref(renderer), textures);
  }

  return renderer;
}

drape_ptr<LayerRenderer> LayerCacher::RecacheCountryStatus(ref_ptr<dp::TextureManager> textures)
{
  m2::PointF surfSize = DrapeGui::Instance().GetSurfaceSize();
  drape_ptr<LayerRenderer> renderer = make_unique_dp<LayerRenderer>();
  CountryStatus countryStatus = CountryStatus(Position(surfSize * 0.5, dp::Center));

  CountryStatus::TButtonHandlers handlers;
  RegisterButtonHandler(handlers, CountryStatusHelper::BUTTON_TYPE_MAP);
  RegisterButtonHandler(handlers, CountryStatusHelper::BUTTON_TYPE_MAP_ROUTING);
  RegisterButtonHandler(handlers, CountryStatusHelper::BUTTON_TRY_AGAIN);

  renderer->AddShapeRenderer(WIDGET_COUNTRY_STATUS, countryStatus.Draw(textures, handlers));
  return renderer;
}

m2::PointF LayerCacher::CacheCompass(Position const & position, ref_ptr<LayerRenderer> renderer,
                                     ref_ptr<dp::TextureManager> textures)
{
  m2::PointF compassSize;
  Compass compass = Compass(position);
  drape_ptr<ShapeRenderer> shape = compass.Draw(compassSize, textures, bind(&DrapeGui::CallOnCompassTappedHandler,
                                                                            &DrapeGui::Instance()));

  renderer->AddShapeRenderer(WIDGET_COMPASS, move(shape));

  return compassSize;
}

m2::PointF LayerCacher::CacheRuler(Position const & position, ref_ptr<LayerRenderer> renderer,
                                   ref_ptr<dp::TextureManager> textures)
{
  m2::PointF rulerSize;
  renderer->AddShapeRenderer(WIDGET_RULER,
                             Ruler(position).Draw(rulerSize, textures));
  return rulerSize;
}

m2::PointF LayerCacher::CacheCopyright(Position const & position, ref_ptr<LayerRenderer> renderer,
                                       ref_ptr<dp::TextureManager> textures)
{
  m2::PointF size;
  renderer->AddShapeRenderer(WIDGET_COPYRIGHT,
                             CopyrightLabel(position).Draw(size, textures));

  return size;
}

m2::PointF LayerCacher::CacheScaleLabel(Position const & position, ref_ptr<LayerRenderer> renderer, ref_ptr<dp::TextureManager> textures)
{
  MutableLabelDrawer::Params params;
  params.m_alphabet = "Scale: 1234567890";
  params.m_maxLength = 10;
  params.m_anchor = position.m_anchor;
  params.m_font = DrapeGui::GetGuiTextFont();
  params.m_pivot = position.m_pixelPivot;
  params.m_handleCreator = [](dp::Anchor, m2::PointF const &)
  {
    return make_unique_dp<ScaleLabelHandle>();
  };

  drape_ptr<ShapeRenderer> scaleRenderer = make_unique_dp<ShapeRenderer>();
  m2::PointF size = MutableLabelDrawer::Draw(params, textures, bind(&ShapeRenderer::AddShape, scaleRenderer.get(), _1, _2));

  renderer->AddShapeRenderer(WIDGET_SCALE_LABLE, move(scaleRenderer));
  return size;
}

}
