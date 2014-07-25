#pragma once

#include <FUi.h>
#include <FUixSensor.h>

class UserMark;
class MapsWithMeForm;

class BookMarkSplitPanel: public Tizen::Ui::Controls::SplitPanel
, public Tizen::Ui::IActionEventListener
, public Tizen::Ui::ITouchEventListener
, public Tizen::Ui::Controls::IListViewItemProviderF
, public Tizen::Ui::Controls::IListViewItemEventListener
, public Tizen::Ui::ITextEventListener
, public Tizen::Uix::Sensor::ISensorEventListener
{
public:
  BookMarkSplitPanel();
  virtual ~BookMarkSplitPanel(void);

  void Enable();
  void Disable();

  bool Construct(Tizen::Graphics::FloatRectangle const & rect);
  void SetMainForm(MapsWithMeForm * pMainForm);
  // IActionEventListener
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);
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
  virtual void OnTouchLongPressed(Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo){}

  //IListViewItemProvider
  virtual Tizen::Ui::Controls::ListItemBase * CreateItem (int index, float itemWidth);
  virtual bool  DeleteItem (int index, Tizen::Ui::Controls::ListItemBase * pItem, float itemWidth);
  virtual int GetItemCount(void);

  Tizen::Ui::Controls::ListItemBase * CreateHeaderItem (float itemWidth);
  Tizen::Ui::Controls::ListItemBase * CreateSettingsItem (float itemWidth);
  Tizen::Ui::Controls::ListItemBase * CreateGroupItem (float itemWidth);
  Tizen::Ui::Controls::ListItemBase * CreateMessageItem (float itemWidth);

  // IListViewItemEventListener
  virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state){}
  virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView & listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
  virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView & listView, int index, Tizen::Ui::Controls::SweepDirection direction){}
  virtual void OnListViewItemLongPressed(Tizen::Ui::Controls::ListView & listView, int index, int elementId, bool & invokeListViewItemCallback){}
  // Tizen::Ui::ITextEventListener
  virtual void  OnTextValueChangeCanceled (Tizen::Ui::Control const & source);
  virtual void  OnTextValueChanged (Tizen::Ui::Control const & source);

  // ISensorEventListener
  virtual void OnDataReceived (Tizen::Uix::Sensor::SensorType sensorType, Tizen::Uix::Sensor::SensorData & sensorData, result r);

  Tizen::Base::String GetHeaderText() const;
  Tizen::Base::String GetDistanceText() const;
  Tizen::Base::String GetCountryText() const;
  Tizen::Base::String GetLocationText() const;
  Tizen::Base::String GetGroupText() const;
  Tizen::Base::String GetMessageText() const;

  void UpdateState();
  UserMark const * GetCurMark() const;
  bool IsBookMark() const;

  void UpdateCompass();
private:

  enum EButtons
  {
    EDIT_BUTTON = 0,
    STAR_BUTTON,
    COMPAS_BACKGROUND_IMG,
    COMPAS_IMG,
    COLOR_IMG,
    DISTANCE_TXT,
    COUNTRY_TXT,
    POSITION_TXT,
    GROUP_TXT,
    MESSAGE_TXT,
    ID_SHARE_BUTTON
  };

  enum EItems
  {
    HEADER_ITEM = 0,
    SETTINGS_ITEM,
    GROUP_ITEM,
    MESSAGE_ITEM
  };

  Tizen::Ui::Controls::Button * m_pButton;
  Tizen::Ui::Controls::Panel * m_pSecondPanel;
  Tizen::Ui::Controls::Label * m_pLabel;
  Tizen::Ui::Controls::EditArea * m_pMessageEdit;
  Tizen::Ui::Controls::EditArea * m_pDummyMessageEdit;

  Tizen::Ui::Controls::ListView * m_pList;
  MapsWithMeForm * m_pMainForm;
  Tizen::Uix::Sensor::SensorManager m_sensorManager;
  double m_northAzimuth;
};
