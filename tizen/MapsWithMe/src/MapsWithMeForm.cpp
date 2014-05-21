#include "MapsWithMeForm.hpp"
#include "MapsWithMeApp.h"
#include "Framework.hpp"
#include "SceneRegister.hpp"
#include "../../../map/framework.hpp"
#include "../../../gui/controller.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../platform/settings.hpp"
#include <FUi.h>
#include <FBase.h>
#include <FMedia.h>
#include <FGraphics.h>

using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Graphics;
using namespace Tizen::Media;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Base::Utility;
using namespace Tizen::Locations;
using namespace Tizen::App;
using namespace Tizen::Ui::Scenes;

MapsWithMeForm::MapsWithMeForm()
:m_pLocProvider(0),
 m_pLabel(0),
 m_pButtonGPS(0),
 m_pButtonSettings(0),
 m_pFramework(0)
{
  SetMultipointTouchEnabled(true);
}

MapsWithMeForm::~MapsWithMeForm(void)
{
  if (m_pLocProvider)
  {
    m_pLocProvider->StopLocationUpdates();
    delete m_pLocProvider;
  }
  if (m_pFramework)
  {
    delete m_pFramework;
  }
}

bool MapsWithMeForm::Initialize(void)
{
  Construct(Tizen::Ui::Controls::FORM_STYLE_NORMAL | FORM_STYLE_PORTRAIT_INDICATOR);
  return true;
}

result MapsWithMeForm::OnInitializing(void)
{
  int width;
  int height;
  width = GetClientAreaBounds().width;
  height = GetClientAreaBounds().height;
  // Create a Label
  m_pLabel = new (std::nothrow) Label();
  m_pLabel->Construct(Rectangle(width / 4, 10, width *3/4, 120), L"GPS off");
  m_pLabel->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
  m_pLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
  AddControl(m_pLabel);
  // Create a Button GPS
  m_pButtonGPS = new (std::nothrow) Button();
  m_pButtonGPS->Construct(Rectangle(50, height -150, 100, 100));
  m_pButtonGPS->SetText(L"GPS\noff");
  m_pButtonGPS->SetActionId(ID_BUTTON_GPS);
  m_pButtonGPS->AddActionEventListener(*this);
  AddControl(m_pButtonGPS);

  // Create a Button Settings
  m_pButtonSettings = new (std::nothrow) Button();
  m_pButtonSettings->Construct(Rectangle(width - 150, height -150, 100, 100));
  m_pButtonSettings->SetText(L"Set\ntings");
  m_pButtonSettings->SetActionId(ID_BUTTON_SETTINGS);
  m_pButtonSettings->AddActionEventListener(*this);
  AddControl(m_pButtonSettings);

  // Create a Button Download
  Button * pButtonDownload = new (std::nothrow) Button();
  pButtonDownload->Construct(Rectangle(width - 270, height -150, 100, 100));
  pButtonDownload->SetText(L"Down\nload");
  pButtonDownload->SetActionId(ID_BUTTON_DOWNLOAD);
  pButtonDownload->AddActionEventListener(*this);
  AddControl(pButtonDownload);

  m_pButtonScalePlus = new (std::nothrow) Button();
  m_pButtonScalePlus->Construct(Rectangle(width - 150, height / 2, 100, 100));
  m_pButtonScalePlus->SetText(L"+");
  m_pButtonScalePlus->SetActionId(ID_BUTTON_SCALE_PLUS);
  m_pButtonScalePlus->AddActionEventListener(*this);
  AddControl(m_pButtonScalePlus);

  m_pButtonScaleMinus = new (std::nothrow) Button();
  m_pButtonScaleMinus->Construct(Rectangle(width - 150,( height / 2) + 120, 100, 100));
  m_pButtonScaleMinus->SetText(L"-");
  m_pButtonScaleMinus->SetActionId(ID_BUTTON_SCALE_MINUS);
  m_pButtonScaleMinus->AddActionEventListener(*this);
  AddControl(m_pButtonScaleMinus);

  UpdateButtons();
  m_locationEnabled = false;

  SetFormBackEventListener(this);

  m_pFramework = new tizen::Framework(this);
  return E_SUCCESS;
}

void MapsWithMeForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  switch(actionId)
  {
    case ID_BUTTON_GPS:
    {
      m_locationEnabled = !m_locationEnabled;
      if (m_locationEnabled)
      {
        LocationCriteria criteria;
        criteria.SetAccuracy(LOC_ACCURACY_FINEST);
        if (m_pLocProvider == 0)
        {
          m_pLocProvider = new LocationProvider();
          m_pLocProvider->Construct(criteria, *this);
        }
        int updateInterval = 1;
        m_pLocProvider->StartLocationUpdatesByInterval(updateInterval);
        m_pLabel->SetText(L"GPS ENABLED");
        m_pButtonGPS->SetText(L"GPS\nON");
        pFramework->StartLocation();
      }
      else
      {
        m_pLocProvider->StopLocationUpdates();
        delete m_pLocProvider;
        m_pLocProvider = 0;
        pFramework->StopLocation();
        m_pLabel->SetText(L"GPS off");
        m_pButtonGPS->SetText(L"GPS\noff");
      }
      break;
    }
    case ID_BUTTON_SCALE_PLUS:
      pFramework->ScaleDefault(true);
      break;
    case ID_BUTTON_SCALE_MINUS:
      pFramework->ScaleDefault(false);
      break;
    case ID_BUTTON_SETTINGS:
    {
      SceneManager * pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoForward(ForwardSceneTransition(SCENE_SETTINGS, SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY, SCENE_DESTROY_OPTION_KEEP));
      break;
    }
    case ID_BUTTON_DOWNLOAD:
    {
      ArrayList * pList = new (std::nothrow) ArrayList;
      pList->Construct();
      pList->Add(*(new (std::nothrow) Integer(0)));
      pList->Add(*(new (std::nothrow) Integer(storage::TIndex::INVALID)));
      pList->Add(*(new (std::nothrow) Integer(storage::TIndex::INVALID)));

      SceneManager * pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoForward(ForwardSceneTransition(SCENE_DOWNLOAD_GROUP,
          SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY, SCENE_DESTROY_OPTION_KEEP), pList);
      break;
    }
  }

  Invalidate(true);
}

namespace detail
{
int ConverToSecondsFrom1970(DateTime const & time)
{
  struct tm y1970;
  y1970.tm_hour = 0;   y1970.tm_min = 0; y1970.tm_sec = 0;
  y1970.tm_year = 1970; y1970.tm_mon = 0; y1970.tm_mday = 1;

  struct tm cur_t;
  cur_t.tm_hour = time.GetHour(); cur_t.tm_min = time.GetMinute(); cur_t.tm_sec = time.GetSecond();
  cur_t.tm_year = time.GetYear(); cur_t.tm_mon = time.GetMonth(); cur_t.tm_mday = time.GetDay();

  return difftime(mktime(&cur_t),mktime(&y1970));
}
}

void MapsWithMeForm::OnLocationUpdated(const Tizen::Locations::Location& location)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  location::GpsInfo info;
  Coordinates const & coord = location.GetCoordinates();
  info.m_source = location::ETizen;
  info.m_timestamp = detail::ConverToSecondsFrom1970(location.GetTimestamp());//!< seconds from 1st Jan 1970
  info.m_latitude = coord.GetLatitude();            //!< degrees
  info.m_longitude = coord.GetLongitude();           //!< degrees
  info.m_horizontalAccuracy = location.GetHorizontalAccuracy();  //!< metres
  info.m_altitude = coord.GetAltitude();            //!< metres
  if (!isnan(location.GetVerticalAccuracy()))
    info.m_verticalAccuracy = location.GetVerticalAccuracy();    //!< metres
  else
    info.m_verticalAccuracy = -1;
  if (!isnan(location.GetCourse()))
    info.m_course = location.GetCourse();             //!< positive degrees from the true North
  else
    info.m_course = -1;
  if (!isnan(location.GetSpeed()))
    info.m_speed = location.GetSpeed() / 3.6;         //!< metres per second
  else
    info.m_speed = -1;

  static int count = 0;
  count++;
  String s = "LocationUpdated ";
  s.Append(count);
  s.Append("\nLat:");
  s.Append(info.m_latitude);
  s.Append(" Lon:");
  s.Append(info.m_longitude);
  s.Append("\nAccuracy:");
  s.Append(info.m_horizontalAccuracy);
  m_pLabel->SetText(s);
  pFramework->OnLocationUpdate(info);
  Draw();
}

void MapsWithMeForm::OnLocationUpdateStatusChanged(Tizen::Locations::LocationServiceStatus status)
{
  static string const ar[5] = {"LOC_SVC_STATUS_IDLE",
      "LOC_SVC_STATUS_RUNNING",
      "LOC_SVC_STATUS_PAUSED",
      "LOC_SVC_STATUS_DENIED",
      "LOC_SVC_STATUS_NOT_FIXED"};
  LOG(LINFO,(ar[status]));
}
void MapsWithMeForm::OnAccuracyChanged(Tizen::Locations::LocationAccuracy accuracy)
{
  static string const ar[6] = {"LOC_ACCURACY_INVALID",
      "LOC_ACCURACY_FINEST",
      "LOC_ACCURACY_TEN_METERS",
      "LOC_ACCURACY_HUNDRED_METERS",
      "LOC_ACCURACY_ONE_KILOMETER",
      "LOC_ACCURACY_ANY"};
  LOG(LINFO,(ar[accuracy]));
}

result MapsWithMeForm::OnDraw(void)
{
  //  m_pFramework->Draw();
  return E_SUCCESS;
}

namespace detail
{
std::vector<std::pair<double, double> > GetTouchedPoints(Tizen::Graphics::Rectangle const & rect)
{
  std::vector<std::pair<double, double> > res;
  IListT<TouchEventInfo *> * pList = TouchEventManager::GetInstance()->GetTouchInfoListN();
  if (pList)
  {
    int count = pList->GetCount();
    for (int i = 0; i < count; ++i)
    {

      TouchEventInfo * pTouchInfo;
      pList->GetAt(i, pTouchInfo);
      Point pt = pTouchInfo->GetCurrentPosition();
      res.push_back(std::make_pair(pt.x - rect.x, pt.y - rect.y));
    }

    pList->RemoveAll();
    delete pList;
  }
  return res;
}
}

void MapsWithMeForm::OnTouchPressed(Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints(GetClientAreaBounds());

  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (!pFramework->GetGuiController()->OnTapStarted(m2::PointD(pts[0].first, pts[0].second)))
  {
    if (pts.size() == 1)
      pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
    else if (pts.size() > 1)
      pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
  }

  std::swap(m_prev_pts, pts);
}

void MapsWithMeForm::OnTouchMoved(Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints(GetClientAreaBounds());
  ::Framework * pFramework = tizen::Framework::GetInstance();

  if (!pFramework->GetGuiController()->OnTapMoved(m2::PointD(pts[0].first, pts[0].second)))
  {
    if (pts.size() == 1 && m_prev_pts.size() > 1)
    {
      pFramework->StopScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
      pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
    }
    else if (pts.size() == 1)
    {
      pFramework->DoDrag(DragEvent(pts[0].first, pts[0].second));
    }
    else if (pts.size() > 1 && m_prev_pts.size() == 1)
    {
      pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
      pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
    else if (pts.size() > 1)
    {
      pFramework->DoScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
  }
  std::swap(m_prev_pts, pts);
}

void MapsWithMeForm::OnTouchReleased(Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints(GetClientAreaBounds());
  ::Framework * pFramework = tizen::Framework::GetInstance();

  //using prev_pts because pts contains not all points
  if (!m_prev_pts.empty())
  {
    if (!pFramework->GetGuiController()->OnTapEnded(m2::PointD(m_prev_pts[0].first, m_prev_pts[0].second)))
    {
      if (m_prev_pts.size() == 1)
        pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
      else if (m_prev_pts.size() > 1)
        pFramework->StopScale(ScaleEvent(m_prev_pts[0].first, m_prev_pts[0].second, m_prev_pts[1].first, m_prev_pts[1].second));
    }
    m_prev_pts.clear();
  }
}

void MapsWithMeForm::OnTouchFocusIn(Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
}

void MapsWithMeForm::OnTouchFocusOut(Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
}

void MapsWithMeForm::OnFormBackRequested(Tizen::Ui::Controls::Form& source)
{
  UiApp * pApp = UiApp::GetInstance();
  AppAssert(pApp);
  pApp->Terminate();
}

void MapsWithMeForm::UpdateButtons()
{
  bool bEnableScaleButtons = true;
  if (!Settings::Get("ScaleButtons", bEnableScaleButtons))
    Settings::Set("ScaleButtons", bEnableScaleButtons);

  m_pButtonScalePlus->SetShowState(bEnableScaleButtons);
  m_pButtonScaleMinus->SetShowState(bEnableScaleButtons);
  Invalidate(true);
}
