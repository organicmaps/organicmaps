#include "map_widget.hpp"

#include "qt/qt_common/helpers.hpp"
#include "qt/qt_common/renderer/renderer_factory.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"

#include "routing/maxspeeds.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <functional>
#include <string>

#include <QTouchEvent>

#include <QtGui/QAction>
#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>

// Fraction of the viewport for a move event
static constexpr float kViewportFractionRoughMove = 0.2;

// Fraction of the viewport for a small move event
static constexpr float kViewportFractionSmoothMove = 0.1;

namespace qt::common
{
// #define ENABLE_AA_SWITCH

MapWidget::MapWidget(Framework & framework, QWidget * parent)
  : QWidget(parent)
  , m_framework(framework)
  , m_slider(nullptr)
  , m_sliderState(SliderState::Released)
  , m_rendererWindow(renderer::RendererFactory::CreateRendererWindow(framework, dp::ApiVersion::OpenGLES3))
  , m_windowContainer(nullptr)
{
  setMouseTracking(true);
  setAttribute(Qt::WA_AcceptTouchEvents);

  m_windowContainer = createWindowContainer(m_rendererWindow, this);
  ASSERT(m_windowContainer != nullptr, ());
  m_windowContainer->setFocusPolicy(Qt::NoFocus);

  auto * layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(m_windowContainer);

  connect(m_rendererWindow, &renderer::base::RendererWindow::OnBeforeEngineCreation, this,
          &MapWidget::BeforeEngineCreation);
  connect(m_rendererWindow, &renderer::base::RendererWindow::OnViewportChanged, this, &MapWidget::OnViewportChanged);
}

MapWidget::~MapWidget() = default;

void MapWidget::BindHotkeys(QWidget & parent)
{
  Hotkey const hotkeys[] = {
      {Qt::Key_Equal, SLOT(ScalePlus())},
      {Qt::Key_Plus, SLOT(ScalePlus())},
      {Qt::Key_Minus, SLOT(ScaleMinus())},
      {Qt::Key_Right, SLOT(MoveRight())},
      {Qt::Key_Left, SLOT(MoveLeft())},
      {Qt::Key_Up, SLOT(MoveUp())},
      {Qt::Key_Down, SLOT(MoveDown())},
      {Qt::ALT | Qt::Key_Equal, SLOT(ScalePlusLight())},
      {Qt::ALT | Qt::Key_Plus, SLOT(ScalePlusLight())},
      {Qt::ALT | Qt::Key_Minus, SLOT(ScaleMinusLight())},
      {Qt::ALT | Qt::Key_Right, SLOT(MoveRightSmooth())},
      {Qt::ALT | Qt::Key_Left, SLOT(MoveLeftSmooth())},
      {Qt::ALT | Qt::Key_Up, SLOT(MoveUpSmooth())},
      {Qt::ALT | Qt::Key_Down, SLOT(MoveDownSmooth())},
#ifdef ENABLE_AA_SWITCH
      {Qt::ALT | Qt::Key_A, SLOT(AntialiasingOn())},
      {Qt::ALT | Qt::Key_S, SLOT(AntialiasingOff())},
#endif
  };

  for (auto const & hotkey : hotkeys)
  {
    auto action = std::make_unique<QAction>(&parent);
    action->setShortcut(QKeySequence(hotkey.m_key));
    connect(action.get(), SIGNAL(triggered()), this, hotkey.m_slot);
    parent.addAction(action.release());
  }
}

void MapWidget::BindSlider(ScaleSlider & slider)
{
  m_slider = &slider;

  connect(m_slider, &QAbstractSlider::actionTriggered, this, &MapWidget::ScaleChanged);
  connect(m_slider, &QAbstractSlider::sliderPressed, this, &MapWidget::SliderPressed);
  connect(m_slider, &QAbstractSlider::sliderReleased, this, &MapWidget::SliderReleased);
}

void MapWidget::ScalePlus()
{
  m_framework.Scale(Framework::SCALE_MAG, true);
}

void MapWidget::ScaleMinus()
{
  m_framework.Scale(Framework::SCALE_MIN, true);
}

void MapWidget::ScalePlusLight()
{
  m_framework.Scale(Framework::SCALE_MAG_LIGHT, true);
}

void MapWidget::ScaleMinusLight()
{
  m_framework.Scale(Framework::SCALE_MIN_LIGHT, true);
}

void MapWidget::MoveRight()
{
  m_framework.Move(-kViewportFractionRoughMove, 0, true);
}

void MapWidget::MoveRightSmooth()
{
  m_framework.Move(-kViewportFractionSmoothMove, 0, true);
}

void MapWidget::MoveLeft()
{
  m_framework.Move(kViewportFractionRoughMove, 0, true);
}

void MapWidget::MoveLeftSmooth()
{
  m_framework.Move(kViewportFractionSmoothMove, 0, true);
}

void MapWidget::MoveUp()
{
  m_framework.Move(0, -kViewportFractionRoughMove, true);
}

void MapWidget::MoveUpSmooth()
{
  m_framework.Move(0, -kViewportFractionSmoothMove, true);
}

void MapWidget::MoveDown()
{
  m_framework.Move(0, kViewportFractionRoughMove, true);
}

void MapWidget::MoveDownSmooth()
{
  m_framework.Move(0, kViewportFractionSmoothMove, true);
}

void MapWidget::AntialiasingOn()
{
  auto engine = m_framework.GetDrapeEngine();
  if (engine != nullptr)
    engine->SetPosteffectEnabled(df::PostprocessRenderer::Antialiasing, true);
}

void MapWidget::AntialiasingOff()
{
  auto engine = m_framework.GetDrapeEngine();
  if (engine != nullptr)
    engine->SetPosteffectEnabled(df::PostprocessRenderer::Antialiasing, false);
}

void MapWidget::ScaleChanged(int action)
{
  if (!m_slider)
    return;

  if (action == QAbstractSlider::SliderNoAction)
    return;

  double const factor = m_slider->GetScaleFactor();
  if (factor != 1.0)
    m_framework.Scale(factor, false);
}

void MapWidget::SliderPressed()
{
  m_sliderState = SliderState::Pressed;
}

void MapWidget::SliderReleased()
{
  m_sliderState = SliderState::Released;
}

void MapWidget::OnViewportChanged(ScreenBase const & screen)
{
  UpdateScaleControl();
}

void MapWidget::UpdateScaleControl()
{
  if (!m_slider || m_sliderState == SliderState::Pressed)
    return;
  m_slider->SetPosWithBlockedSignals(m_framework.GetDrawScale());
}

namespace
{
search::ReverseGeocoder::Address GetFeatureAddressInfo(Framework const & framework, FeatureType & ft)
{
  search::ReverseGeocoder const coder(framework.GetDataSource());
  search::ReverseGeocoder::Address address;
  coder.GetExactAddress(ft, address);
  return address;
}
}  // namespace

void MapWidget::ShowInfoPopup(QMouseEvent * e, m2::PointD const & pt)
{
  // show feature types
  QMenu menu;
  auto const addStringFn = [&menu](std::string const & s)
  { return s.empty() ? nullptr : menu.addAction(QString::fromUtf8(s.c_str())); };

  for (int scale : {scales::GetUpperScale(), scales::GetUpperWorldScale()})
  {
    m_framework.ForEachFeatureAtPoint([&](FeatureType & ft)
    {
      // ID
      QAction * pAction = addStringFn(DebugPrint(ft.GetID()));
      connect(pAction, &QAction::triggered, std::bind(&Framework::ShowFeature, &m_framework, ft.GetID()));

      // Types
      std::string concat;
      auto types = feature::TypesHolder(ft);
      types.SortBySpec();
      for (auto const & type : types.ToObjectNames())
        concat = concat + type + " ";
      concat = concat + "| " + DebugPrint(ft.GetGeomType());
      addStringFn(concat);

      // Name + Ref
      std::string name(ft.GetReadableName());
      if (auto const & ref = ft.GetRef(); !ref.empty())
      {
        if (!name.empty())
          name += " | ";
        name += ref;
      }
      addStringFn(name);

      // Address
      auto const info = GetFeatureAddressInfo(m_framework, ft);
      addStringFn(info.FormatAddress());

      if (ft.GetGeomType() == feature::GeomType::Line)
      {
        // Maxspeed
        auto const & dataSource = m_framework.GetDataSource();
        auto const handle = dataSource.GetMwmHandleById(ft.GetID().m_mwmId);
        auto const speeds = routing::LoadMaxspeeds(handle);
        if (speeds)
        {
          auto const speed = speeds->GetMaxspeed(ft.GetID().m_index);
          if (speed.IsValid())
            addStringFn(DebugPrint(speed));
        }
      }

      int const layer = ft.GetLayer();
      if (layer != feature::LAYER_EMPTY)
        addStringFn("Layer = " + std::to_string(layer));

      menu.addSeparator();
    }, m_framework.PtoG(pt), scale);
  }

  menu.exec(e->pos());
}
}  // namespace qt::common
