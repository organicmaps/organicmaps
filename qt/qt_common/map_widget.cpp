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

#include <QLayout>
#include <QTouchEvent>
#include <QtGui/QPainter>

#include <QtGui/QAction>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenu>

// Fraction of the viewport for a move event
static constexpr float kViewportFractionRoughMove = 0.2;

// Fraction of the viewport for a small move event
static constexpr float kViewportFractionSmoothMove = 0.1;

namespace qt::common
{
// #define ENABLE_AA_SWITCH

namespace
{
class OverlayWidget final : public QWidget
{
public:
  explicit OverlayWidget(QWidget * parent) : QWidget(parent)
  {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setAutoFillBackground(false);
  }

protected:
  void paintEvent(QPaintEvent * event) override
  {
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    event->accept();
  }
};
}  // namespace

MapWidget::MapWidget(Framework & framework, QWidget * parent)
  : QWidget(parent)
  , m_framework(framework)
  , m_slider(nullptr)
  , m_sliderState(SliderState::Released)
  , m_rendererWindow(nullptr)
  , m_rootLayout(nullptr)
  , m_windowContainer(nullptr)
  , m_overlayWindow(nullptr)
{
  setMouseTracking(true);
  setAttribute(Qt::WA_AcceptTouchEvents);

  m_rootLayout = new QGridLayout(this);
  m_rootLayout->setContentsMargins(0, 0, 0, 0);
  m_rootLayout->setSpacing(0);
  m_rootLayout->setRowStretch(0, 1);
  m_rootLayout->setColumnStretch(0, 1);
  setLayout(m_rootLayout);

  m_overlayWindow = new OverlayWidget(this);
  m_rootLayout->addWidget(m_overlayWindow, 0, 0, Qt::AlignCenter);
  SetRenderingApi(m_framework.LoadPreferredGraphicsAPI());
}

MapWidget::~MapWidget()
{
  delete m_overlayWindow;
  m_overlayWindow = nullptr;
}

void MapWidget::SetOverlayLayout(QLayout * layout) const
{
  if (layout == nullptr)
    return;

  if (QLayout * oldLayout = m_overlayWindow->layout(); oldLayout != nullptr)
    delete oldLayout;

  layout->setContentsMargins(0, 0, 0, 0);
  m_overlayWindow->setLayout(layout);
  m_overlayWindow->adjustSize();
  m_overlayWindow->show();
  m_overlayWindow->raise();
}

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

void MapWidget::SetRenderingApi(dp::ApiVersion const apiVersion)
{
  if (m_rendererWindow != nullptr && m_rendererWindow->GetApiVersion() == apiVersion)
    return;

  if (m_windowContainer)
  {
    m_rootLayout->removeWidget(m_windowContainer);
    delete m_windowContainer;
    m_windowContainer = nullptr;
  }

  m_rendererWindow = renderer::RendererFactory::CreateRendererWindow(m_framework, apiVersion);
  m_windowContainer = createWindowContainer(m_rendererWindow);
  ASSERT(m_windowContainer != nullptr, ());
  m_windowContainer->setFocusPolicy(Qt::NoFocus);

  m_windowContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_rootLayout->addWidget(m_windowContainer, 0, 0);

  connect(m_rendererWindow, &renderer::base::RendererWindow::OnBeforeEngineCreation, this,
          &MapWidget::OnBeforeEngineCreation);
  connect(m_rendererWindow, &renderer::base::RendererWindow::OnAfterEngineCreation, this,
          &MapWidget::OnAfterEngineCreation);
  connect(m_rendererWindow, &renderer::base::RendererWindow::OnViewportChanged, this, &MapWidget::OnViewportChanged);

  m_rendererWindow->SetEventReceiver(this);
}

void MapWidget::OnBeforeEngineCreation()
{
  emit BeforeEngineCreation();
}

void MapWidget::OnAfterEngineCreation()
{
  emit AfterEngineCreation();
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

int MapWidget::L2D(int const px) const
{
  return static_cast<int>(px * m_rendererWindow->GetRatio());
}

m2::PointD MapWidget::GetDevicePoint(QMouseEvent const * const e) const
{
  return m2::PointD(L2D(e->position().x()), L2D(e->position().y()));
}

df::Touch MapWidget::GetDfTouchFromQMouseEvent(QMouseEvent const * const e) const
{
  df::Touch touch;
  touch.m_id = 0;
  touch.m_location = GetDevicePoint(e);
  return touch;
}

df::TouchEvent MapWidget::GetDfTouchEventFromQMouseEvent(QMouseEvent const * const e,
                                                         df::TouchEvent::ETouchType const type) const
{
  df::TouchEvent event;
  event.SetTouchType(type);
  event.SetFirstTouch(GetDfTouchFromQMouseEvent(e));
  if (IsCommandModifier(e))
    event.SetSecondTouch(GetSymmetrical(event.GetFirstTouch()));

  return event;
}

df::Touch MapWidget::GetSymmetrical(df::Touch const & touch) const
{
  m2::PointD const pixelCenter = m_framework.GetVisiblePixelCenter();
  m2::PointD const symmetricalLocation = pixelCenter + pixelCenter - m2::PointD(touch.m_location);

  df::Touch result;
  result.m_id = touch.m_id + 1;
  result.m_location = symmetricalLocation;

  return result;
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent * e)
{
  QWidget::mouseDoubleClickEvent(e);
  if (IsLeftButton(e))
    m_framework.Scale(Framework::SCALE_MAG_LIGHT, GetDevicePoint(e), true);
}

void MapWidget::mousePressEvent(QMouseEvent * e)
{
  QWidget::mousePressEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetDfTouchEventFromQMouseEvent(e, df::TouchEvent::TOUCH_DOWN));
}

void MapWidget::mouseMoveEvent(QMouseEvent * e)
{
  QWidget::mouseMoveEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetDfTouchEventFromQMouseEvent(e, df::TouchEvent::TOUCH_MOVE));
}

void MapWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (IsRightButton(e))
    emit OnContextMenuRequested(e->globalPosition().toPoint());

  QWidget::mouseReleaseEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetDfTouchEventFromQMouseEvent(e, df::TouchEvent::TOUCH_UP));
}

void MapWidget::wheelEvent(QWheelEvent * e)
{
  QWidget::wheelEvent(e);

  QPointF const pos = e->position();

  double const factor = e->angleDelta().y() / 3.0 / 360.0;
  // https://doc-snapshots.qt.io/qt6-dev/qwheelevent.html#angleDelta, angleDelta() returns in eighths of a degree.
  m_framework.Scale(exp(factor), m2::PointD(L2D(pos.x()), L2D(pos.y())), false);
}
}  // namespace qt::common
