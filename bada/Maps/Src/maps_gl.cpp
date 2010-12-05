#include "../../../base/SRC_FIRST.hpp"

#include "maps_gl.h"
#include "maps_form.h"
#include "maps_control.h"
#include "render_context.hpp"

#include "../../../map/drawer_yg.hpp"
#include "../../../platform/platform.hpp"
#include "../../../geometry/rect2d.hpp"

#include <FBase.h>

using namespace Osp::Graphics::Opengl;

MapsGl::MapsGl(MapsControl * app, MapsForm * form)
: __pApplication(app), __pForm(form)
{
}

MapsGl::~MapsGl()
{}

bool MapsGl::Init()
{
  int x, y, width, height;
  __pApplication->GetAppFrame()->GetFrame()->GetBounds(x, y, width, height);

  m_primaryContext = make_shared_ptr(new bada::RenderContext(__pForm));
  m_primaryContext->makeCurrent();

  m_drawer = make_shared_ptr(new DrawerYG(GetPlatform().ResourcesDir() + "basic.skn", 10000, 30000));
  m_drawer->onSize(width, height);

  m_windowHandle = make_shared_ptr(new bada::WindowHandle(__pForm));
  m_windowHandle->setDrawer(m_drawer);

  m_drawer->screen()->addOnFlushFinishedFn(glFinish);

  m_framework = make_shared_ptr(new framework_t(m_windowHandle));
  m_framework->Init();
  m_framework->OnSize(width, height);
  //m_framework->setPrimaryContext(m_primaryContext);
  return true;
}

void MapsGl::Draw()
{
  shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_windowHandle->drawer()));
  m_windowHandle->drawer()->beginFrame();
  m_framework->PaintImpl(paintEvent, m_framework->Screen());
  m_windowHandle->drawer()->endFrame();
//  m_framework->Paint(paintEvent);
}
