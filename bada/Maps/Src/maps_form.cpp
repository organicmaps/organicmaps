#include "maps_form.h"
#include "maps_gl.h"
#include "download_form.h"

#include <FMedia.h>

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Graphics;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::App;
using namespace Osp::Locations;

MapsForm::MapsForm() : m_mapsGl(null), m_locProvider(null), m_pDownloadForm(null),
    _pFrame(null), _pOptionMenu(null),
      _pPopup(null), _pBtn(null), _pLabel(null),
      _pPopup3(null), _pBtn3(null), _pLabel3(null)
{
}

MapsForm::~MapsForm()
{
  // everything should be in OnTerminating()
}

result MapsForm::OnInitializing()
{
  return E_SUCCESS;
}

result MapsForm::OnTerminating()
{
  if (m_locProvider)
{
    m_locProvider->CancelLocationUpdates();
    delete m_locProvider;
  }
  if (m_pDownloadForm)
  {
    _pFrame->RemoveControl(*m_pDownloadForm);
  }
  delete _pOptionMenu;
  delete _pPopup;
  delete _pPopup3;

  return E_SUCCESS;
}

result MapsForm::CreateForm(Frame* pFrame)
{
  result r = E_SUCCESS;
  Point pt(0, 0);
  Dimension dim(450, 250);
  _pFrame = pFrame;

  r = Form::Construct(FORM_STYLE_NORMAL | FORM_STYLE_TITLE
      | FORM_STYLE_INDICATOR | FORM_STYLE_SOFTKEY_0 | FORM_STYLE_SOFTKEY_1
      | FORM_STYLE_OPTIONKEY);
  if (IsFailed(r))
  {
    AppLog("Form::Construct() has failed.");
    return E_FAILURE;
  }
  r = SetTitleText(L"MapsWithMe");
  SetName("MapsForm");
  r = pFrame->AddControl(*this);

  // modify Softkeys
  SetSoftkeyText(SOFTKEY_0, L"+");
  SetSoftkeyActionId(SOFTKEY_0, ACTION_ID_ZOOM_IN);
  AddSoftkeyActionListener(SOFTKEY_0, *this);
  SetSoftkeyText(SOFTKEY_1, L"-");
  SetSoftkeyActionId(SOFTKEY_1, ACTION_ID_ZOOM_OUT);
  AddSoftkeyActionListener(SOFTKEY_1, *this);

  AddOptionkeyActionListener(*this);
  SetOptionkeyActionId(ACTION_ID_MENU);

  AddTouchEventListener(*this);
  Touch touch;
  touch.SetMultipointEnabled(*this, true);

  AddOrientationEventListener(*this);
  SetOrientation(ORIENTATION_PORTRAIT/*ORIENTATION_AUTOMATIC_FOUR_DIRECTION*/);

//  CreatePopups();

  return E_SUCCESS;
}

//void MapsForm::CreatePopups()
//{
//  // 1. popup for notifying drawing error
//  _pPopup = new Popup();
//  _pPopup->Construct(false, Dimension(450, 240));
//
//  _pLabel = new Label();
//  _pLabel->Construct(Rectangle(0, 0, 450, 110), L"An error occurred\nwhen drawing the map.");
//  _pLabel->SetTextConfig(35, LABEL_TEXT_STYLE_NORMAL);
//
//  _pBtn = new Button();
//  _pBtn->Construct(Rectangle(450/2-90, 240/2+15, 180, 70),L"Close");
//  _pBtn->SetActionId(ACTION_ID_BUTTON_CLOSE_POPUP);
//  _pBtn->AddActionEventListener(*this);
//
//  _pPopup->AddControl(*_pLabel);
//  _pPopup->AddControl(*_pBtn);
//
//  // 2. popup for setting location services
//      _pPopup3 = new Popup();
//      _pPopup3->Construct(false, Dimension(450, 335));
//
//      _pLabel3 = new Label();
//      _pLabel3->Construct(Rectangle(0, 0, 450, 220),L"Location services may be disabled. To enable them, go\nto Settings > Connectivity > Location and switch on\n\"Enable location services\".");
//	_pLabel3->SetTextConfig(35, LABEL_TEXT_STYLE_NORMAL);
//
//	_pBtn3 = new Button();
//	_pBtn3->Construct(Rectangle(450/2-90, 300/2+80, 180, 70),L"OK");
//	_pBtn3->SetActionId(ACTION_ID_BUTTON_OK_POPUP);
//	_pBtn3->AddActionEventListener(*this);
//
//	_pPopup3->AddControl(*_pLabel3);
//	_pPopup3->AddControl(*_pBtn3);
//}

result MapsForm::OnDraw()
{
  m_mapsGl->Draw();
  return E_SUCCESS;
}

void MapsForm::OnActionPerformed(const Osp::Ui::Control &source, int actionId)
{
  switch (actionId)
  {
  case ACTION_ID_MENU:
  {
    // Display the Popup on the display
    ShowOptionMenu();
  }
    break;

  case ACTION_ID_ZOOM_IN:
  {
    m_mapsGl->Framework()->ScaleDefault(true);
  }
    break;

  case ACTION_ID_ZOOM_OUT:
  {
    m_mapsGl->Framework()->ScaleDefault(false);
  }
    break;

  case ACTION_ID_MY_LOCATION:
  {
      if (!m_locProvider)
      { // Create a LocationProvider
        m_locProvider = new LocationProvider();
        // Construct the LocationProvider by using GPS as its locating mechanism
        if (IsFailed(m_locProvider->Construct(LOC_METHOD_HYBRID)))
        {
            delete m_locProvider;
            m_locProvider = null;
        }
      }

      if (m_locProvider)
        m_locProvider->RequestLocationUpdates(*this, 5, false);
  }
    break;

  case ACTION_ID_ALL:
  {
    m_mapsGl->Framework()->ShowAll();
  }
    break;

    case ACTION_ID_DOWNLOAD_MAPS:
    {
      if (m_pDownloadForm == null)
      {
        m_pDownloadForm = new DownloadForm(*m_mapsGl);
        m_pDownloadForm->Initialize();
        _pFrame->AddControl(*m_pDownloadForm);
      }
      _pFrame->SetCurrentForm(*m_pDownloadForm);
      m_pDownloadForm->Draw();
      m_pDownloadForm->Show();
    }
    break;
  }
}

void MapsForm::ShowOptionMenu(void)
{
  // Create a OptionMenu
  if (null == _pOptionMenu)
  {
    _pOptionMenu = new OptionMenu();
    _pOptionMenu->Construct();
    _pOptionMenu->AddActionEventListener(*this);

    _pOptionMenu->AddItem(L"My Position", ACTION_ID_MY_LOCATION);
    _pOptionMenu->AddItem(L"Countries", ACTION_ID_DOWNLOAD_MAPS);
//    _pOptionMenu->AddItem(L"Show All", ACTION_ID_ALL);
  }
  else
  {
    _pOptionMenu->SetShowState(true);
  }

  _pOptionMenu->Show();
}

void MapsForm::OnOrientationChanged(const Osp::Ui::Control& source,
    Osp::Ui::OrientationStatus orientationStatus)
{
}

void MapsForm::OnTouchDoublePressed(const Control &source,
    const Point &currentPosition, const TouchEventInfo &touchInfo)
{
  AppLog("OnTouchDoublePressed");
}

void MapsForm::OnTouchFocusIn(const Control &source,
    const Point &currentPosition, const TouchEventInfo &touchInfo)
{
  AppLog("OnTouchFocusIn");
}

void MapsForm::OnTouchFocusOut(const Control &source,
    const Point &currentPosition, const TouchEventInfo &touchInfo)
{
  AppLog("OnTouchFocusOut");
}

void MapsForm::OnTouchLongPressed(const Control &source,
    const Point &currentPosition, const TouchEventInfo &touchInfo)
{
  AppLog("OnTouchLongPressed");
}

void MapsForm::OnTouchMoved(const Osp::Ui::Control &source,
    const Point &currentPosition, const TouchEventInfo &touchInfo)
{
  AppLog("OnTouchMoved");
  m_mapsGl->Framework()->DoDrag(DragEvent(currentPosition.x, currentPosition.y));
}

void MapsForm::OnTouchPressed(const Control &source,
    const Point &currentPosition, const TouchEventInfo &touchInfo)
{
  AppLog("OnTouchPressed");
  m_mapsGl->Framework()->StartDrag(DragEvent(currentPosition.x, currentPosition.y));
}

void MapsForm::OnTouchReleased(const Control &source,
    const Point &currentPosition, const TouchEventInfo &touchInfo)
{
  AppLog("OnTouchReleased");
  m_mapsGl->Framework()->StopDrag(DragEvent(currentPosition.x, currentPosition.y));
}

void MapsForm::OnUserEventReceivedN(RequestId requestId, Osp::Base::Collection::IList* pArgs)
{
  // Hide Download Maps dialog
  if (requestId == DownloadForm::REQUEST_MAINFORM)
  {
    Frame * pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();
    pFrame->SetCurrentForm(*this);
    Draw();
    Show();
    pFrame->RemoveControl(*m_pDownloadForm);
    m_pDownloadForm = null;
  }
}

void MapsForm::OnLocationUpdated(Osp::Locations::Location & location)
{
  QualifiedCoordinates const * coords = location.GetQualifiedCoordinates();
  if (coords)
  {
    // The lat and lon values
    double const lon = coords->GetLongitude();
    double const lat = coords->GetLatitude();
    AppLog("Received location: %lf, %lf", lat, lon);
    m2::PointD const pt(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
    m_mapsGl->Framework()->CenterViewport(pt);
    m_locProvider->CancelLocationUpdates();
  }
}

void MapsForm::OnProviderStateChanged(Osp::Locations::LocProviderState newState)
{
}
