#pragma once

#include <FUi.h>
#include <FUiITouchEventListener.h>
#include <FLocations.h>
#include "../../../std/vector.hpp"

namespace tizen
{
class Framework;
}

class MapsWithMeForm
: public Tizen::Ui::Controls::Form
  , public Tizen::Ui::ITouchEventListener
  , public Tizen::Ui::IActionEventListener
  , public Tizen::Locations::ILocationProviderListener
  , public Tizen::Ui::Controls::IFormBackEventListener
  , public Tizen::Ui::Controls::IListViewItemProviderF
  , public Tizen::Ui::Controls::IListViewItemEventListener
  {
  public:
  MapsWithMeForm();
  virtual ~MapsWithMeForm(void);

  virtual result OnDraw(void);
  bool Initialize(void);
  virtual result OnInitializing(void);

  // ITouchEventListener
  virtual void  OnTouchFocusIn (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void  OnTouchFocusOut (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void  OnTouchMoved (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void  OnTouchPressed (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void  OnTouchReleased (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);

  // IActionEventListener
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);

  // ILocationProviderListener
  virtual void OnLocationUpdated(Tizen::Locations::Location const & location);
  virtual void OnLocationUpdateStatusChanged(Tizen::Locations::LocationServiceStatus status);
  virtual void OnAccuracyChanged(Tizen::Locations::LocationAccuracy accuracy);

  // IFormBackEventListener
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form& source);

  //IListViewItemProvider
  virtual Tizen::Ui::Controls::ListItemBase * CreateItem (int index, float itemWidth);
  virtual bool  DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth);
  virtual int GetItemCount(void);
  // IListViewItemEventListener
  virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state);
  virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
  virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView & listView, int index, Tizen::Ui::Controls::SweepDirection direction);
  virtual void OnListViewItemLongPressed(Tizen::Ui::Controls::ListView & listView, int index, int elementId, bool & invokeListViewItemCallback);

  void UpdateButtons();
  void ShowSplitPanel();
  void HideSplitPanel();

  private:
  bool m_locationEnabled;
  std::vector<std::pair<double, double> > m_prev_pts;

  enum
  {
    ID_GPS = 101,
    ID_SEARCH,
    ID_MENU,
    ID_STAR,
    ID_BUTTON_SCALE_PLUS,
    ID_BUTTON_SCALE_MINUS
  } EEventIDs;

  enum
  {
    eDownloadProVer = 0,
    eDownloadMaps,
    eSettings,
    eSharePlace
  } EMainMenuItems;

  Tizen::Locations::LocationProvider * m_pLocProvider;

  Tizen::Ui::Controls::Button * m_pButtonScalePlus;
  Tizen::Ui::Controls::Button * m_pButtonScaleMinus;

  Tizen::Ui::Controls::SplitPanel * m_pSplitPanel;
  Tizen::Ui::Controls::Panel* m_pFirstPanel;
  Tizen::Ui::Controls::Panel* m_pSecondPanel;

  tizen::Framework * m_pFramework;
  };
