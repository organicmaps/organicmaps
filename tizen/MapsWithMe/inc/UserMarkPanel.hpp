#pragma once

#include <FUi.h>

class UserMark;
class MapsWithMeForm;

class UserMarkPanel: public Tizen::Ui::Controls::Panel
, public Tizen::Ui::IActionEventListener
, public Tizen::Ui::ITouchEventListener
{
public:
  UserMarkPanel();
  virtual ~UserMarkPanel(void);
  void Enable();
  void Disable();

  bool Construct(const Tizen::Graphics::FloatRectangle& rect);
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

  void UpdateState();
  UserMark const * GetCurMark();
private:
  MapsWithMeForm * m_pMainForm;
  Tizen::Ui::Controls::Label * m_pLabel;
  Tizen::Ui::Controls::Button * m_pButton;
  enum EActions
  {
    ID_STAR
  };
};
