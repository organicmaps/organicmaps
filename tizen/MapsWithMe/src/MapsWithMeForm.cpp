#include "MapsWithMeForm.hpp"
#include "MapsWithMeApp.h"
#include "Framework.hpp"
#include "../../../map/framework.hpp"
#include "../../../gui/controller.hpp"
#include "../../../platform/tizen_string_utils.hpp"
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

MapsWithMeForm::MapsWithMeForm(MapsWithMeApp* pApp)
:m_pLocProvider(0),
 m_pLabel(0),
 m_pButton(0),
 m_pApp(pApp)
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
}

bool MapsWithMeForm::Initialize(void)
{
  LOG(LDEBUG, ("MapsWithMeForm::Initialize"));
  Construct(Tizen::Ui::Controls::FORM_STYLE_NORMAL);
  return true;
}

result MapsWithMeForm::OnInitializing(void)
{
  LOG(LDEBUG, ("MapsWithMeForm::OnInitializing"));

  int width;
  int height;
  GetSize(width, height);
  // Create a Label
  m_pLabel = new (std::nothrow) Label();
  m_pLabel->Construct(Rectangle(width / 4, 10, width *3/4, 120), L"GPS off");
  m_pLabel->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
  m_pLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
  AddControl(m_pLabel);
  // Create a Button
  m_pButton = new (std::nothrow) Button();
  m_pButton->Construct(Rectangle(width - 150, height -150, 100, 100));
  m_pButton->SetText(L"GPS\noff");
  m_pButton->SetActionId(ID_BUTTON);
  m_pButton->AddActionEventListener(*this);
  AddControl(m_pButton);

  m_locationEnabled = false;
  return E_SUCCESS;
}

void MapsWithMeForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  switch(actionId)
  {
    case ID_BUTTON:
    {
      ::Framework * pFramework = tizen::Framework::GetInstance();
      m_locationEnabled = !m_locationEnabled;
      if (m_locationEnabled)
      {
        LocationCriteria criteria;
        criteria.SetAccuracy(LOC_ACCURACY_FINEST);
        //criteria.SetAccuracy(LOC_ACCURACY_TEN_METERS);
        //criteria.SetAccuracy(LOC_ACCURACY_ANY);
        if (m_pLocProvider == 0)
        {
          m_pLocProvider = new LocationProvider();
          m_pLocProvider->Construct(criteria, *this);
        }
        int updateInterval = 1;
        m_pLocProvider->StartLocationUpdatesByInterval(updateInterval);
        //double distanceThreshold = 1.0;
        //m_pLocProvider->StartLocationUpdatesByDistance(distanceThreshold);
        m_pLabel->SetText(L"GPS ENABLED");
        m_pButton->SetText(L"GPS\nON");
        pFramework->StartLocation();
      }
      else
      {
        m_pLocProvider->StopLocationUpdates();
        pFramework->StopLocation();
        m_pLabel->SetText(L"GPS off");
        m_pButton->SetText(L"GPS\noff");
      }
    }
    break;
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
  cur_t.tm_hour = time.GetHour();   cur_t.tm_min = time.GetMinute(); cur_t.tm_sec = time.GetSecond();
  cur_t.tm_year = time.GetYear(); cur_t.tm_mon = time.GetMonth(); cur_t.tm_mday = time.GetDay();

  return difftime(mktime(&cur_t),mktime(&y1970));
}
}

void MapsWithMeForm::OnLocationUpdated(Tizen::Locations::Location const & location)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  location::GpsInfo info;
  Coordinates const & coord = location.GetCoordinates();
  info.m_source = location::ETizen;
  info.m_timestamp = detail::ConverToSecondsFrom1970(location.GetTimestamp());//!< seconds from 1st Jan 1970
  info.m_latitude = coord.GetLatitude();              //!< degrees
  info.m_longitude = coord.GetLongitude();            //!< degrees
  info.m_horizontalAccuracy = location.GetHorizontalAccuracy();  //!< metres
  info.m_altitude = coord.GetAltitude();              //!< metres
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
#ifdef _DEBUG
  static string const ar[5] = {"LOC_SVC_STATUS_IDLE",
      "LOC_SVC_STATUS_RUNNING",
      "LOC_SVC_STATUS_PAUSED",
      "LOC_SVC_STATUS_DENIED",
      "LOC_SVC_STATUS_NOT_FIXED"};
  LOG(LDEBUG,(ar[status]));
#endif
}
void MapsWithMeForm::OnAccuracyChanged(Tizen::Locations::LocationAccuracy accuracy)
{
#ifdef _DEBUG
  static string const ar[6] = {"LOC_ACCURACY_INVALID",
      "LOC_ACCURACY_FINEST",
      "LOC_ACCURACY_TEN_METERS",
      "LOC_ACCURACY_HUNDRED_METERS",
      "LOC_ACCURACY_ONE_KILOMETER",
      "LOC_ACCURACY_ANY"};
  LOG(LDEBUG,(ar[accuracy]));
#endif
}

result MapsWithMeForm::OnDraw(void)
{
  return m_pApp->Draw();
}

namespace detail
{
std::vector<std::pair<double, double> > GetTouchedPoints()
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
      res.push_back(std::make_pair(pt.x, pt.y));
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
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints();

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
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints();
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
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints();
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
