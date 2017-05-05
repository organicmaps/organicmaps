#include "qt/qt_common/map_widget.hpp"

#include "qt/qt_common/helpers.hpp"
#include "qt/qt_common/scale_slider.hpp"

#include "map/framework.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#include <QtWidgets/QAction>

namespace qt
{
namespace common
{
MapWidget::MapWidget(Framework & framework, QWidget * parent)
  : QOpenGLWidget(parent)
  , m_framework(framework)
  , m_slider(nullptr)
  , m_sliderState(SliderState::Released)
  , m_ratio(1.0)
  , m_contextFactory(nullptr)
{
  QSurfaceFormat fmt = format();

  fmt.setMajorVersion(2);
  fmt.setMinorVersion(1);

  fmt.setAlphaBufferSize(8);
  fmt.setBlueBufferSize(8);
  fmt.setGreenBufferSize(8);
  fmt.setRedBufferSize(8);
  fmt.setStencilBufferSize(0);
  fmt.setSamples(0);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setSwapInterval(1);
  fmt.setDepthBufferSize(16);
  fmt.setProfile(QSurfaceFormat::CompatibilityProfile);

  // fmt.setOption(QSurfaceFormat::DebugContext);
  setFormat(fmt);
  setMouseTracking(true);

  // Update widget contents each 30ms.
  m_updateTimer = make_unique<QTimer>(this);
  VERIFY(connect(m_updateTimer.get(), SIGNAL(timeout()), this, SLOT(update())), ());
  m_updateTimer->setSingleShot(false);
  m_updateTimer->start(30);
}

MapWidget::~MapWidget()
{
  m_framework.EnterBackground();
  m_framework.SetRenderingDisabled(true);
  m_contextFactory->PrepareToShutdown();
  m_framework.DestroyDrapeEngine();
  m_contextFactory.reset();
}

void MapWidget::BindHotkeys(QWidget & parent)
{
  Hotkey const hotkeys[] = {
      {Qt::Key_Equal, SLOT(ScalePlus())},
      {Qt::Key_Minus, SLOT(ScaleMinus())},
      {Qt::ALT + Qt::Key_Equal, SLOT(ScalePlusLight())},
      {Qt::ALT + Qt::Key_Minus, SLOT(ScaleMinusLight())},
  };

  for (auto const & hotkey : hotkeys)
  {
    auto action = make_unique<QAction>(&parent);
    action->setShortcut(QKeySequence(hotkey.m_key));
    connect(action.get(), SIGNAL(triggered()), this, hotkey.m_slot);
    parent.addAction(action.release());
  }
}

void MapWidget::BindSlider(ScaleSlider & slider)
{
  m_slider = &slider;

  connect(m_slider, SIGNAL(actionTriggered(int)), this, SLOT(ScaleChanged(int)));
  connect(m_slider, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
  connect(m_slider, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
}

void MapWidget::CreateEngine()
{
  Framework::DrapeCreationParams p;
  p.m_apiVersion = dp::ApiVersion::OpenGLES2;
  p.m_surfaceWidth = m_ratio * width();
  p.m_surfaceHeight = m_ratio * height();
  p.m_visualScale = m_ratio;

  m_skin.reset(new gui::Skin(gui::ResolveGuiSkinFile("default"), m_ratio));
  m_skin->Resize(p.m_surfaceWidth, p.m_surfaceHeight);
  m_skin->ForEach(
      [&p](gui::EWidget widget, gui::Position const & pos) { p.m_widgetsInitInfo[widget] = pos; });

  p.m_widgetsInitInfo[gui::WIDGET_SCALE_LABEL] = gui::Position(dp::LeftBottom);

  m_framework.CreateDrapeEngine(make_ref(m_contextFactory), std::move(p));
  m_framework.SetViewportListener([this](ScreenBase const & /* screen */) {
    UpdateScaleControl();
  });
}

void MapWidget::ScalePlus() { m_framework.Scale(Framework::SCALE_MAG, true); }

void MapWidget::ScaleMinus() { m_framework.Scale(Framework::SCALE_MIN, true); }

void MapWidget::ScalePlusLight() { m_framework.Scale(Framework::SCALE_MAG_LIGHT, true); }

void MapWidget::ScaleMinusLight() { m_framework.Scale(Framework::SCALE_MIN_LIGHT, true); }

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

void MapWidget::SliderPressed() { m_sliderState = SliderState::Pressed; }

void MapWidget::SliderReleased() { m_sliderState = SliderState::Released; }

m2::PointD MapWidget::GetDevicePoint(QMouseEvent * e) const
{
  return m2::PointD(L2D(e->x()), L2D(e->y()));
}

df::Touch MapWidget::GetTouch(QMouseEvent * e) const
{
  df::Touch touch;
  touch.m_id = 0;
  touch.m_location = GetDevicePoint(e);
  return touch;
}

df::TouchEvent MapWidget::GetTouchEvent(QMouseEvent * e, df::TouchEvent::ETouchType type) const
{
  df::TouchEvent event;
  event.SetTouchType(type);
  event.SetFirstTouch(GetTouch(e));
  if (IsCommandModifier(e))
    event.SetSecondTouch(GetSymmetrical(event.GetFirstTouch()));

  return event;
}

df::Touch MapWidget::GetSymmetrical(df::Touch const & touch) const
{
  m2::PointD const pixelCenter = m_framework.GetPixelCenter();
  m2::PointD const symmetricalLocation = pixelCenter + (pixelCenter - touch.m_location);

  df::Touch result;
  result.m_id = touch.m_id + 1;
  result.m_location = symmetricalLocation;

  return result;
}

void MapWidget::UpdateScaleControl()
{
  if (!m_slider || m_sliderState == SliderState::Pressed)
    return;
  m_slider->SetPosWithBlockedSignals(m_framework.GetDrawScale());
}

void MapWidget::initializeGL()
{
  ASSERT(m_contextFactory == nullptr, ());
  m_ratio = devicePixelRatio();
  m_contextFactory.reset(new QtOGLContextFactory(context()));

  emit BeforeEngineCreation();
  CreateEngine();
  m_framework.EnterForeground();
}

void MapWidget::paintGL()
{
  static QOpenGLShaderProgram * program = nullptr;
  if (program == nullptr)
  {
    const char * vertexSrc =
        "\
      attribute vec2 a_position; \
      attribute vec2 a_texCoord; \
      uniform mat4 u_projection; \
      varying vec2 v_texCoord; \
      \
      void main() \
      { \
        gl_Position = u_projection * vec4(a_position, 0.0, 1.0);\
        v_texCoord = a_texCoord; \
      }";

    const char * fragmentSrc =
        "\
      uniform sampler2D u_sampler; \
      varying vec2 v_texCoord; \
      \
      void main() \
      { \
        gl_FragColor = vec4(texture2D(u_sampler, v_texCoord).rgb, 1.0); \
      }";

    program = new QOpenGLShaderProgram(this);
    program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSrc);
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSrc);
    program->link();
  }

  if (m_contextFactory->LockFrame())
  {
    QOpenGLFunctions * funcs = context()->functions();
    funcs->glActiveTexture(GL_TEXTURE0);
    GLuint image = m_contextFactory->GetTextureHandle();
    funcs->glBindTexture(GL_TEXTURE_2D, image);

    int projectionLocation = program->uniformLocation("u_projection");
    int samplerLocation = program->uniformLocation("u_sampler");

    QMatrix4x4 projection;
    QRect r = rect();
    r.setWidth(m_ratio * r.width());
    r.setHeight(m_ratio * r.height());
    projection.ortho(r);

    program->bind();
    program->setUniformValue(projectionLocation, projection);
    program->setUniformValue(samplerLocation, 0);

    float const w = m_ratio * width();
    float h = m_ratio * height();

    QVector2D positions[4] = {QVector2D(0.0, 0.0), QVector2D(w, 0.0), QVector2D(0.0, h),
                              QVector2D(w, h)};

    QRectF const & texRect = m_contextFactory->GetTexRect();
    QVector2D texCoords[4] = {QVector2D(texRect.bottomLeft()), QVector2D(texRect.bottomRight()),
                              QVector2D(texRect.topLeft()), QVector2D(texRect.topRight())};

    program->enableAttributeArray("a_position");
    program->enableAttributeArray("a_texCoord");
    program->setAttributeArray("a_position", positions, 0);
    program->setAttributeArray("a_texCoord", texCoords, 0);

    funcs->glClearColor(0.0, 0.0, 0.0, 1.0);
    funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    funcs->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_contextFactory->UnlockFrame();
  }
}

void MapWidget::resizeGL(int width, int height)
{
  float w = m_ratio * width;
  float h = m_ratio * height;
  m_framework.OnSize(w, h);
  m_framework.SetVisibleViewport(m2::RectD(0, 0, w, h));
  if (m_skin)
  {
    m_skin->Resize(w, h);

    gui::TWidgetsLayoutInfo layout;
    m_skin->ForEach(
        [&layout](gui::EWidget w, gui::Position const & pos) { layout[w] = pos.m_pixelPivot; });

    m_framework.SetWidgetLayout(std::move(layout));
  }
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent * e)
{
  QOpenGLWidget::mouseDoubleClickEvent(e);
  if (IsLeftButton(e))
    m_framework.Scale(Framework::SCALE_MAG_LIGHT, GetDevicePoint(e), true);
}

void MapWidget::mousePressEvent(QMouseEvent * e)
{
  QOpenGLWidget::mousePressEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_DOWN));
}

void MapWidget::mouseMoveEvent(QMouseEvent * e)
{
  QOpenGLWidget::mouseMoveEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_MOVE));
}

void MapWidget::mouseReleaseEvent(QMouseEvent * e)
{
  QOpenGLWidget::mouseReleaseEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetTouchEvent(e, df::TouchEvent::TOUCH_UP));
}

void MapWidget::wheelEvent(QWheelEvent * e)
{
  QOpenGLWidget::wheelEvent(e);
  m_framework.Scale(exp(e->delta() / 360.0), m2::PointD(L2D(e->x()), L2D(e->y())), false);
}
}  // namespace common
}  // namespace qt
