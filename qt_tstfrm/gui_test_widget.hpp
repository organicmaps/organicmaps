#pragma once

#include "gl_test_widget.hpp"
#include "../../gui/controller.hpp"
#include "../../base/strings_bundle.hpp"
#include <QMouseEvent>

template <class T, void (T::*)(gui::Controller*)>
struct init_with_controller_fn_bind
{
  typedef T type;
};

template <class T, class U>
struct has_init_with_controller
{
  static bool const value = false;
};

template <class T>
struct has_init_with_controller<T, typename init_with_controller_fn_bind<T, &T::Init>::type>
{
  static bool const value = true;
};

template <typename TTest>
class GUITestWidget : public GLTestWidget<TTest>
{
private:

  typedef GLTestWidget<TTest> base_t;

  shared_ptr<gui::Controller> m_controller;
  shared_ptr<graphics::Screen> m_cacheScreen;
  shared_ptr<StringsBundle> m_stringBundle;

public:

  void invalidate()
  {
    base_t::updateGL();
  }

  void initializeGL()
  {
    base_t::initializeGL();

    m_controller.reset(new gui::Controller());

    gui::Controller::RenderParams rp;

    graphics::Screen::Params cp;

    cp.m_doUnbindRT = false;
    cp.m_threadSlot = 0;
    cp.m_storageType = graphics::ETinyStorage;
    cp.m_textureType = graphics::ESmallTexture;
    cp.m_isSynchronized = false;
    cp.m_resourceManager = base_t::m_resourceManager;
    cp.m_renderContext = base_t::m_primaryContext;

    m_cacheScreen = make_shared_ptr(new graphics::Screen(cp));

    rp.m_CacheScreen = m_cacheScreen.get();
    rp.m_GlyphCache = base_t::m_resourceManager->glyphCache(0);
    rp.m_InvalidateFn = bind(&GUITestWidget<TTest>::invalidate, this);
    rp.m_Density = graphics::EDensityMDPI;

    m_stringBundle.reset(new StringsBundle());
    m_stringBundle->SetDefaultString("country_status_download", "Download^");

    m_controller->SetStringsBundle(m_stringBundle.get());

    InitImpl(m_controller, bool_tag<has_init_with_controller<TTest, TTest>::value >());

    m_controller->SetRenderParams(rp);
  }

  void InitImpl(shared_ptr<gui::Controller> const & c, bool_tag<true> const &)
  {
    base_t::test.Init(c.get());
  }

  void InitImpl(shared_ptr<gui::Controller> const & c, bool_tag<false> const &)
  {}

  void DoDraw(shared_ptr<graphics::Screen> const & s)
  {
    base_t::DoDraw(s);
    m_controller->UpdateElements();
    m_controller->DrawFrame(s.get());
  }

  void mousePressEvent(QMouseEvent * e)
  {
    base_t::mousePressEvent(e);

    if (e->button() == Qt::LeftButton)
      m_controller->OnTapStarted(m2::PointU(e->pos().x(), e->pos().y()));
  }

  void mouseReleaseEvent(QMouseEvent * e)
  {
    base_t::mouseReleaseEvent(e);

    if (e->button() == Qt::LeftButton)
      m_controller->OnTapEnded(m2::PointU(e->pos().x(), e->pos().y()));
  }

  void mouseMoveEvent(QMouseEvent * e)
  {
    base_t::mouseMoveEvent(e);

    m_controller->OnTapMoved(m2::PointU(e->pos().x(), e->pos().y()));
  }
};

template <class Test> QWidget * create_gui_test_widget()
{
  GUITestWidget<Test> * w = new GUITestWidget<Test>();
  w->Init();
  return w;
}

#define UNIT_TEST_GUI(name)\
  void UnitTestGUI_##name();\
  TestRegister g_TestRegisterGUI_##name("Test::"#name, __FILE__, &UnitTestGUI_##name);\
  void UnitTestGUI_##name()\
  {\
    char * argv[] = { const_cast<char *>(#name) };\
    int argc = 1;\
    QApplication app(argc, argv);\
    QWidget * w = create_gui_test_widget<name>();\
    w->show();\
    app.exec();\
  }
