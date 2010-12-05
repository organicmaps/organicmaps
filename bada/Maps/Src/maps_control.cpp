#include "maps_control.h"
#include "maps_form.h"
#include "maps_gl.h"

#include <FUi.h>
#include <FMedia.h>

using namespace Osp::Base;
using namespace Osp::Graphics;
using namespace Osp::Locales;
using namespace Osp::System;
using namespace Osp::App;
using namespace Osp::Ui::Controls;

MapsControl::MapsControl() : __glWrapper(null)
{
}

MapsControl::~MapsControl()
{
  // cleanup should be implemented inside OnAppTerminating()
}

Application * MapsControl::CreateInstance(void)
{
  // You can create the instance through another constructor.
  return new MapsControl();
}

bool MapsControl::OnAppInitializing(AppRegistry& appRegistry)
{
  result r = E_SUCCESS;

  IAppFrame* pAppFrame = GetAppFrame();
  MapsForm* pMapPanel = new MapsForm();

  if (NULL == pAppFrame)
  {
    AppLog("GetAppFrame() has failed.");
    goto CATCH;
  }

  if (NULL == pMapPanel)
  {
    AppLog("Unable to create MapForm");
    goto CATCH;
  }

  r = pMapPanel->CreateForm(pAppFrame->GetFrame());
  if (IsFailed(r))
  {
    AppLog("__pMapPanel->CreateForm() has failed.");
    goto CATCH;
  }

  // initialize OpenGl
   __glWrapper = new MapsGl(this, pMapPanel);
   __glWrapper->Init();

   pMapPanel->m_mapsGl = __glWrapper;

  // Uncomment the following statement to listen to the screen on/off events.
  //PowerManager::SetScreenEventListener(*this);



  return true;

  CATCH: if (pMapPanel != null)
  {
    delete pMapPanel;
    pMapPanel = null;
  }

  return false;
}

bool MapsControl::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
  // Deallocate or close any resources still alive.
  // Save the application's current states, if applicable.
  // If this method is successful, return true; otherwise, return false.

  if (__glWrapper)
  {
    __glWrapper->Framework()->SaveState();
    delete __glWrapper;
    __glWrapper = 0;
  }

  return true;
}

void MapsControl::OnForeground(void)
{
}

void MapsControl::OnBackground(void)
{
}

void MapsControl::OnLowMemory(void)
{
  // TODO:
  // Deallocate as many resources as possible.
}

void MapsControl::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
  // TODO:
  // It is recommended that the application save its data,
  // and terminate itself if the application consumes much battery.
}

void MapsControl::OnScreenOn(void)
{
  // TODO:
  // Get the released resources or resume the operations that were paused or stopped in OnScreenOff().
}

void MapsControl::OnScreenOff(void)
{
  // TODO:
  //  Unless there is a strong reason to do otherwise, release resources (such as 3D, media, and sensors) to allow the device to enter the sleep mode to save the battery.
  // Invoking a lengthy asynchronous method within this listener method can be risky, because it is not guaranteed to invoke a callback before the device enters the sleep mode.
  // Similarly, do not perform lengthy operations in this listener method. Any operation must be a quick one.
}

void MapsControl::Draw()
{
  __glWrapper->Draw();
}
