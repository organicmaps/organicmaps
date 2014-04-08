#pragma once
#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>

//  The HelloWorldApp class must inherit from the UiApp class,
//  which provides the basic features necessary to define a UI application.

class HelloWorldApp: public Tizen::App::UiApp, public Tizen::System::IScreenEventListener
{
public:
  // The platform calls this method to create the application instance.
  static Tizen::App::UiApp* CreateInstance(void);
  HelloWorldApp(void);
  virtual ~HelloWorldApp(void);

private:
  // Called when the UI application is initializing.
  virtual bool OnAppInitializing(Tizen::App::AppRegistry& appRegistry);

  // Called when the UI application initialization is finished.
  virtual bool OnAppInitialized(void);

  // Called when the END key is pressed to terminate the UI application (if the device has the END key).
  virtual bool OnAppWillTerminate(void);

  // Called when the UI application is terminating.
  virtual bool OnAppTerminating(Tizen::App::AppRegistry& appRegistry, bool forcedTermination = false);

  // Called when the UI application's frame moves to the foreground.
  virtual void OnForeground(void);

  // Called when the UI application's frame is moved from the foreground to the background.
  virtual void OnBackground(void);

  // Called when system memory is no longer sufficient to run the UI application. Clean up unnecessary resources to release memory, or terminate the application.
  virtual void OnLowMemory(void);

  // Called when the battery level changes.
  virtual void OnBatteryLevelChanged(Tizen::System::BatteryLevel batteryLevel);

  // Called when the screen switches on.
  virtual void OnScreenOn(void);

  // Called when the screen switches off.
  virtual void OnScreenOff(void);
};
