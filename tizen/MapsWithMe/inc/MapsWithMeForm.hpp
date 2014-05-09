#pragma once

#include <FUi.h>
#include <FUiITouchEventListener.h>
#include <FLocations.h>
#include "../../../std/vector.hpp"

class MapsWithMeApp;

class MapsWithMeForm
: public Tizen::Ui::Controls::Form
  , public Tizen::Ui::ITouchEventListener
  , public Tizen::Ui::IActionEventListener
  , public Tizen::Locations::ILocationProviderListener
{
public:
  MapsWithMeForm(MapsWithMeApp* pApp);
  virtual ~MapsWithMeForm(void);

  virtual result OnDraw(void);
  bool Initialize(void);
  virtual result OnInitializing(void);

  // ITouchEventListener
  virtual void  OnTouchFocusIn (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchFocusOut (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchMoved (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchPressed (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchReleased (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);

  // IActionEventListener
  virtual void OnActionPerformed(const Tizen::Ui::Control& source, int actionId);

  // ILocationProviderListener
  virtual void OnLocationUpdated(const Tizen::Locations::Location& location);
  virtual void OnLocationUpdateStatusChanged(Tizen::Locations::LocationServiceStatus status);
  virtual void OnAccuracyChanged(Tizen::Locations::LocationAccuracy accuracy);
private:

  bool m_locationEnabled;
  std::vector<std::pair<double, double> > m_prev_pts;
  static const int ID_BUTTON = 101;

  Tizen::Locations::LocationProvider * m_pLocProvider;
  Tizen::Ui::Controls::Label * m_pLabel;
  Tizen::Ui::Controls::Button * m_pButton;
  MapsWithMeApp* m_pApp;
};
