#include "MapsWithMeApp.h"
#include "MapsWithMeFrame.h"
#include "MapsWithMeForm.hpp"
#include "SettingsForm.hpp"
#include "Framework.hpp"
#include "../../../base/logging.hpp"
#include <FUi.h>

using namespace Tizen::App;
using namespace Tizen::System;

MapsWithMeApp::MapsWithMeApp(void) {}

MapsWithMeApp::~MapsWithMeApp(void) {}

UiApp * MapsWithMeApp::CreateInstance(void) {return new MapsWithMeApp();}

bool MapsWithMeApp::OnAppInitializing(AppRegistry& appRegistry)
{
  // TODO: Initialize application-specific data.
  // The permanent data and context of the application can be obtained from the application registry (appRegistry).
  //
  // If this method is successful, return true; otherwise, return false and the application is terminated.


  // Uncomment the following statement to listen to the screen on and off events:
  // PowerManager::SetScreenEventListener(*this);


  // Create the application frame.
  MapsWithMeFrame* pMapsWithMeFrame = new MapsWithMeFrame;
  TryReturn(pMapsWithMeFrame != null, false, "The memory is insufficient.");
  pMapsWithMeFrame->Construct();
  pMapsWithMeFrame->SetName(L"MapsWithMe");
  AddFrame(*pMapsWithMeFrame);

  return true;
}

result MapsWithMeApp::Draw()
{
  return E_SUCCESS;
}

bool MapsWithMeApp::OnAppInitialized(void) {return true;}

bool MapsWithMeApp::OnAppWillTerminate(void)
{
  // TODO: Deallocate or release resources in devices that have the END key.
  return true;
}

bool MapsWithMeApp::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
  // TODO: Deallocate all resources allocated by the application.
  // The permanent data and context of the application can be saved through the application registry (appRegistry).
  return true;
}

void MapsWithMeApp::OnForeground(void)
{
  // TODO: Start or resume drawing when the application is moved to the foreground.
}

void MapsWithMeApp::OnBackground(void)
{
  // TODO: Stop drawing when the application is moved to the background to save the CPU and battery consumption.
}

void MapsWithMeApp::OnLowMemory(void)
{
  // TODO: Free unnecessary resources or close the application.
}

void MapsWithMeApp::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
  // TODO: Handle all battery level changes here.
  // Stop using multimedia features (such as camera and mp3 playback) if the battery level is CRITICAL.
}

void MapsWithMeApp::OnScreenOn(void)
{
  // TODO: Retrieve the released resources or resume the operations that were paused or stopped in the OnScreenOff() event handler.
}

void MapsWithMeApp::OnScreenOff(void)
{
  // TODO: Release resources (such as 3D, media, and sensors) to allow the device to enter the sleep mode
  // to save the battery (unless you have a good reason to do otherwise).
  // Only perform quick operations in this event handler. Any lengthy operations can be risky;
  // for example, invoking a long asynchronous method within this event handler can cause problems
  // because the device can enter the sleep mode before the callback is invoked.
}
