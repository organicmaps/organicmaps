#pragma once

#include <FUi.h>
#include <FUiITouchEventListener.h>
#include <FLocations.h>
#include "../../../map/user_mark.hpp"
#include "TouchProcessor.hpp"

namespace tizen
{
class Framework;
}

class UserMarkPanel;
class BookMarkSplitPanel;

class MapsWithMeForm
: public Tizen::Ui::Controls::Form
  , public Tizen::Ui::ITouchEventListener
  , public Tizen::Ui::IActionEventListener
  , public Tizen::Locations::ILocationProviderListener
  , public Tizen::Ui::Controls::IFormBackEventListener
  , public Tizen::Ui::Controls::IListViewItemProviderF
  , public Tizen::Ui::Controls::IListViewItemEventListener
  , public Tizen::Ui::Scenes::ISceneEventListener
  , public Tizen::Ui::Controls::IFormMenuEventListener
  , public Tizen::Ui::Controls::ISearchBarEventListener
  , public Tizen::Ui::ITextEventListener
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
      Tizen::Ui::TouchEventInfo const & touchInfo){}
  virtual void  OnTouchFocusOut (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo){}
  virtual void  OnTouchMoved (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo){}
  virtual void  OnTouchPressed (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void  OnTouchReleased (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo){}

  // IActionEventListener
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);

  // ILocationProviderListener
  virtual void OnLocationUpdated(Tizen::Locations::Location const & location);
  virtual void OnLocationUpdateStatusChanged(Tizen::Locations::LocationServiceStatus status);
  virtual void OnAccuracyChanged(Tizen::Locations::LocationAccuracy accuracy);

  // IFormBackEventListener
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  // IFormMenuEventListener
  virtual void OnFormMenuRequested(Tizen::Ui::Controls::Form & source);

  //IListViewItemProvider
  virtual Tizen::Ui::Controls::ListItemBase * CreateItem (int index, float itemWidth);
  virtual bool  DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth);
  virtual int GetItemCount(void);
  // IListViewItemEventListener
  virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state){}
  virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
  virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView & listView, int index, Tizen::Ui::Controls::SweepDirection direction){}
  virtual void OnListViewItemLongPressed(Tizen::Ui::Controls::ListView & listView, int index, int elementId, bool & invokeListViewItemCallback){}
  // ISceneEventListener
  virtual void OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId & previousSceneId,
                   const Tizen::Ui::Scenes::SceneId & currentSceneId, Tizen::Base::Collection::IList * pArgs);
  virtual void OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId & currentSceneId,
                  const Tizen::Ui::Scenes::SceneId & nextSceneId){}
  // ISearchBarEventListener
  virtual void OnSearchBarModeChanged(Tizen::Ui::Controls::SearchBar & source, Tizen::Ui::Controls::SearchBarMode mode);
  // ITextEventListener
  virtual void OnTextValueChangeCanceled(const Tizen::Ui::Control & source){}
  virtual void OnTextValueChanged(const Tizen::Ui::Control & source);

  //IUserMarkListener
  void OnUserMark(UserMarkCopy * pCopy);
  void OnDismissListener();

  void UpdateButtons();

  void CreateSplitPanel();
  void ShowSplitPanel();
  void HideSplitPanel();
  bool m_splitPanelEnabled;

  void CreateBookMarkPanel();
  void ShowBookMarkPanel();
  void HideBookMarkPanel();
  bool m_bookMarkPanelEnabled;

  void CreateBookMarkSplitPanel();
  void ShowBookMarkSplitPanel();
  void HideBookMarkSplitPanel();
  void UpdateBookMarkSplitPanelState();
  bool m_bookMArkSplitPanelEnabled;

  void CreateSearchBar();
  void ShowSearchBar();
  void HideSearchBar();
  bool m_searchBarEnabled;
  Tizen::Base::String m_searchText;

private:
  bool m_locationEnabled;

  enum EEventIDs
  {
    ID_GPS = 101,
    ID_SEARCH,
    ID_MENU,
    ID_STAR,
    ID_BUTTON_SCALE_PLUS,
    ID_BUTTON_SCALE_MINUS,
    ID_SEARCH_CANCEL
  };

  enum EMainMenuItems
  {
    //eDownloadProVer = 0,
    eDownloadMaps,
    eSettings,
    eSharePlace
  };

  Tizen::Locations::LocationProvider * m_pLocProvider;

  Tizen::Ui::Controls::Button * m_pButtonScalePlus;
  Tizen::Ui::Controls::Button * m_pButtonScaleMinus;

  Tizen::Ui::Controls::SplitPanel * m_pSplitPanel;
  Tizen::Ui::Controls::Panel* m_pFirstPanel;
  Tizen::Ui::Controls::Panel* m_pSecondPanel;
  Tizen::Ui::Controls::SearchBar * m_pSearchBar;

  UserMarkPanel * m_userMarkPanel;
  BookMarkSplitPanel * m_bookMarkSplitPanel;

  tizen::Framework * m_pFramework;

  TouchProcessor m_touchProcessor;
};
