#pragma once

#include <FBase.h>
#include <FGraphics.h>
#include <FApp.h>
#include <FUiControls.h>
#include <FSystem.h>

class MapsGl;

class MapsControl: public Osp::App::Application,
    public Osp::System::IScreenEventListener
{
  MapsGl * __glWrapper;

public:
  // The application must have a factory method that creates an instance of the application.
  static Osp::App::Application* CreateInstance(void);

public:
  MapsControl();
  ~MapsControl();

public:
  // This method is called when the application is on initializing.
  bool OnAppInitializing(Osp::App::AppRegistry& appRegistry);

  // This method is called when the application is on terminating.
  bool OnAppTerminating(Osp::App::AppRegistry& appRegistry,
      bool forcedTermination = false);

  // This method is called when the application is brought to the foreground
  void OnForeground(void);

  // This method is called when the application is sent to the background.
  void OnBackground(void);

  // This method is called when the application has little available memory.
  void OnLowMemory(void);

  // This method is called when the device's battery level is changed.
  void OnBatteryLevelChanged(Osp::System::BatteryLevel batteryLevel);

  //	Called when the screen turns on.
  void OnScreenOn(void);

  //	Called when the screen turns off.
  void OnScreenOff(void);

  void Draw();
};
