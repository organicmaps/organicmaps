#pragma once

#include <FBase.h>
#include <FGraphics.h>
#include <FUiControls.h>
#include <FApp.h>
#include <FLocations.h>

class MapsGl;
class DownloadForm;

class MapsForm: public Osp::Ui::Controls::Form,
    public Osp::Ui::IActionEventListener,
    public Osp::Ui::IOrientationEventListener,
    public Osp::Ui::ITouchEventListener,
    public Osp::Locations::ILocationListener
{
public:
  MapsForm();
  ~MapsForm();

private:
  enum
  {
    ACTION_ID_MENU = 10000,
    ACTION_ID_ZOOM_IN,
    ACTION_ID_ZOOM_OUT,
    ACTION_ID_MY_LOCATION,
    ACTION_ID_DOWNLOAD_MAPS,
    ACTION_ID_ALL
  };

  // listener functions
public:
  // TODO: temporary, fix later
  MapsGl * m_mapsGl;

  // control lifetime events
  virtual result OnInitializing();
  virtual result OnTerminating();

  // button action event
  virtual void OnActionPerformed(const Osp::Ui::Control &source, int actionId);

  // screen orientation event
  virtual void OnOrientationChanged(const Osp::Ui::Control& source,
      Osp::Ui::OrientationStatus orientationStatus);

  // touch events
  virtual void  OnTouchDoublePressed (const Osp::Ui::Control &source, const Osp::Graphics::Point &currentPosition, const Osp::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchFocusIn (const Osp::Ui::Control &source, const Osp::Graphics::Point &currentPosition, const Osp::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchFocusOut (const Osp::Ui::Control &source, const Osp::Graphics::Point &currentPosition, const Osp::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchLongPressed (const Osp::Ui::Control &source, const Osp::Graphics::Point &currentPosition, const Osp::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchMoved (const Osp::Ui::Control &source, const Osp::Graphics::Point &currentPosition, const Osp::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchPressed (const Osp::Ui::Control &source, const Osp::Graphics::Point &currentPosition, const Osp::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchReleased (const Osp::Ui::Control &source, const Osp::Graphics::Point &currentPosition, const Osp::Ui::TouchEventInfo &touchInfo);

  // location events
  virtual void  OnLocationUpdated(Osp::Locations::Location & location);
  virtual void  OnProviderStateChanged(Osp::Locations::LocProviderState newState);

  virtual void OnUserEventReceivedN(RequestId requestId, Osp::Base::Collection::IList* pArgs);

public:
  result CreateForm(Osp::Ui::Controls::Frame * pFrame);

private:
  virtual result OnDraw(void);
  void ShowOptionMenu(void);

private:
  Osp::Locations::LocationProvider * m_locProvider;

  DownloadForm * m_pDownloadForm;

  Osp::Ui::Controls::Frame * _pFrame;
  Osp::Ui::Controls::OptionMenu * _pOptionMenu;

  // pop up for remind to set the location service
  Osp::Ui::Controls::Popup * _pPopup;
  Osp::Ui::Controls::Button * _pBtn;
  Osp::Ui::Controls::Label * _pLabel;

  // pop up for setting
  Osp::Ui::Controls::Popup * _pPopup3;
  Osp::Ui::Controls::Button * _pBtn3;
  Osp::Ui::Controls::Label * _pLabel3;
};
