#include "Framework.hpp"
#include "RenderContext.hpp"
#include "VideoTimer.hpp"

#include <FBase.h>
#include <FUi.h>

#include "../../../map/framework.hpp"
#include "../../../map/render_policy.hpp"

#include "../../../graphics/defines.hpp"
#include "../../../graphics/data_formats.hpp"

#include "../../../platform/platform.hpp"

#include "../../../std/bind.hpp"

namespace tizen
{
::Framework* Framework::m_Instance = 0;

Framework::Framework(Tizen::Ui::Controls::Form * form)
{
  tizen::RenderContext * pContext = new tizen::RenderContext();
  if (!pContext->Init(form))
  {
    LOG(LINFO, ("Context Init failed"));
    /// ToDo
  }
  pContext->makeCurrent();
  m_context.reset(pContext);

  setlocale(LC_NUMERIC, "C");
  ::Framework * pFramework = GetInstance();
  pFramework->LoadBookmarks();
  VideoTimer1 * m_VideoTimer = new VideoTimer1(bind(&Framework::Draw, this));
  RenderPolicy::Params params;
  params.m_screenHeight = form->GetBounds().height - form->GetClientAreaBounds().y;
  params.m_screenWidth = form->GetClientAreaBounds().width;
  params.m_useDefaultFB = true;
  params.m_skinName = "basic.skn";
  params.m_density = graphics::EDensityXHDPI; // todo
  params.m_videoTimer = m_VideoTimer;
  params.m_primaryRC = static_pointer_cast<graphics::RenderContext>(m_context);

  graphics::ResourceManager::Params rm_params;
  rm_params.m_texFormat = graphics::Data4Bpp;
  rm_params.m_texRtFormat = graphics::Data4Bpp;
  rm_params.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();
  params.m_rmParams = rm_params;

  try
  {
    pFramework->SetRenderPolicy(CreateRenderPolicy(params));
    pFramework->InitGuiSubsystem();
  }
  catch (graphics::gl::platform_unsupported & except)
  {
  }
  LOG(LINFO, ("width ", params.m_screenWidth, " height ",params.m_screenHeight));
  pFramework->OnSize(params.m_screenWidth, params.m_screenHeight);
  pFramework->SetUpdatesEnabled(true);
  pFramework->AddLocalMaps();
  if (!pFramework->LoadState())
    pFramework->ShowAll();
}

Framework::~Framework()
{
  ::Framework * pFramework = GetInstance();
  pFramework->SaveState();
  //delete m_VideoTimer;  ?? ERROR on timer destructor...
}

::Framework* Framework::GetInstance()
{
  if (!m_Instance)
    m_Instance = new ::Framework();
  return m_Instance;
}

void Framework::Draw()
{
  ::Framework * pFramework = GetInstance();
  if (pFramework->NeedRedraw())
  {
    //    makeCurrent();
    pFramework->SetNeedRedraw(false);

    shared_ptr<PaintEvent> paintEvent(new PaintEvent(pFramework->GetRenderPolicy()->GetDrawer().get()));

    pFramework->BeginPaint(paintEvent);
    pFramework->DoPaint(paintEvent);

    // swapping buffers before ending the frame, see issue #333
    //    swapBuffers();

    pFramework->EndPaint(paintEvent);
    //    doneCurrent();
    m_context->SwapBuffers();
  }


}
}
