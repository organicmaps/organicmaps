#include "Framework.hpp"
#include "VideoTimer.hpp"

#include "../core/jni_helper.hpp"
#include "../core/render_context.hpp"

#include "../../../../../map/framework.hpp"

#include "../../../../../gui/controller.hpp"

#include "../../../../../indexer/drawing_rules.hpp"

#include "../../../../../coding/file_container.hpp"
#include "../../../../../coding/file_name_utils.hpp"

#include "../../../../../graphics/opengl/framebuffer.hpp"
#include "../../../../../graphics/opengl/opengl.hpp"

#include "../../../../../platform/platform.hpp"
#include "../../../../../platform/location.hpp"

#include "../../../../../base/math.hpp"
#include "../../../../../base/logging.hpp"


const unsigned LONG_TOUCH_MS = 1000;
const unsigned SHORT_TOUCH_MS = 250;
const double DOUBLE_TOUCH_S = SHORT_TOUCH_MS / 1000.0;

android::Framework * g_framework = 0;


namespace android
{
  void Framework::CallRepaint()
  {
    //LOG(LINFO, ("Calling Repaint"));
  }

  Framework::Framework()
   : m_mask(0),
     m_isCleanSingleClick(false),
     m_doLoadState(true),
     m_wasLongClick(false)
  {
    ASSERT_EQUAL ( g_framework, 0, () );
    g_framework = this;

    m_videoTimer = new VideoTimer(bind(&Framework::CallRepaint, this));
    size_t const measurementsCount = 5;
    m_sensors[0].SetCount(measurementsCount);
    m_sensors[1].SetCount(measurementsCount);

    m_bmType = "placemark-red";

    for (size_t i = 0; i < ARRAY_SIZE(m_images); ++i)
      m_images[i] = 0;
  }

  Framework::~Framework()
  {
    for (size_t i = 0; i < ARRAY_SIZE(m_images); ++i)
      delete m_images[i];

    delete m_videoTimer;
  }

  void Framework::OnLocationError(int errorCode)
  {
    m_work.OnLocationError(static_cast<location::TLocationError>(errorCode));
  }

  void Framework::OnLocationUpdated(uint64_t time, double lat, double lon, float accuracy)
  {
    location::GpsInfo info;
    info.m_timestamp = static_cast<double>(time);
    info.m_latitude = lat;
    info.m_longitude = lon;
    info.m_horizontalAccuracy = accuracy;
    m_work.OnLocationUpdate(info);
  }

  void Framework::OnCompassUpdated(uint64_t timestamp, double magneticNorth, double trueNorth, double accuracy)
  {
    location::CompassInfo info;
    info.m_timestamp = static_cast<double>(timestamp);
    info.m_magneticHeading = magneticNorth;
    info.m_trueHeading = trueNorth;
    info.m_accuracy = accuracy;
    m_work.OnCompassUpdate(info);
  }

  void Framework::UpdateCompassSensor(int ind, float * arr)
  {
    //LOG ( LINFO, ("Sensors before, C++: ", arr[0], arr[1], arr[2]) );
    m_sensors[ind].Next(arr);
    //LOG ( LINFO, ("Sensors after, C++: ", arr[0], arr[1], arr[2]) );
  }

  void Framework::DeleteRenderPolicy()
  {
    m_work.SaveState();
    LOG(LINFO, ("Clearing current render policy."));
    m_work.SetRenderPolicy(0);
    m_work.EnterBackground();
  }

  bool Framework::InitRenderPolicy(int densityDpi, int screenWidth, int screenHeight)
  {
    graphics::ResourceManager::Params rmParams;

    rmParams.m_videoMemoryLimit = 30 * 1024 * 1024;
    rmParams.m_rtFormat = graphics::Data8Bpp;
    rmParams.m_texFormat = graphics::Data4Bpp;

    RenderPolicy::Params rpParams;

    rpParams.m_videoTimer = m_videoTimer;
    rpParams.m_useDefaultFB = true;
    rpParams.m_rmParams = rmParams;
    rpParams.m_primaryRC = make_shared_ptr(new android::RenderContext());

    char const * suffix = 0;
    switch (densityDpi)
    {
    case 120:
      rpParams.m_density = graphics::EDensityLDPI;
      break;

    case 160:
      rpParams.m_density = graphics::EDensityMDPI;
      break;

    case 240:
      rpParams.m_density = graphics::EDensityHDPI;
      break;

    default:
      rpParams.m_density = graphics::EDensityXHDPI;
      break;
    }

    rpParams.m_skinName = "basic.skn";
    LOG(LINFO, ("Using", graphics::convert(rpParams.m_density), "resources"));

    rpParams.m_screenWidth = screenWidth;
    rpParams.m_screenHeight = screenHeight;

    try
    {
      m_work.SetRenderPolicy(CreateRenderPolicy(rpParams));
      if (m_doLoadState)
        LoadState();
      else
        m_doLoadState = true;
    }
    catch (graphics::gl::platform_unsupported const & e)
    {
      LOG(LINFO, ("This android platform is unsupported, reason:", e.what()));
      return false;
    }

    graphics::EDensity const density = m_work.GetRenderPolicy()->Density();
    m_images[IMAGE_PLUS] = new ImageT("plus.png", density);
    m_images[IMAGE_ARROW] = new ImageT("right-arrow.png", density);

    m_work.SetUpdatesEnabled(true);
    m_work.EnterForeground();
    return true;
  }

  storage::Storage & Framework::Storage()
  {
    return m_work.Storage();
  }

  CountryStatusDisplay * Framework::GetCountryStatusDisplay()
  {
    return m_work.GetCountryStatusDisplay();
  }

  void Framework::ShowCountry(storage::TIndex const & idx)
  {
    m_doLoadState = false;

    m_work.ShowCountry(idx);
  }

  storage::TStatus Framework::GetCountryStatus(storage::TIndex const & idx) const
  {
    return m_work.GetCountryStatus(idx);
  }

  void Framework::DeleteCountry(storage::TIndex const & idx)
  {
    m_work.DeleteCountry(idx);
  }

  void Framework::Resize(int w, int h)
  {
    m_work.OnSize(w, h);
  }

  void Framework::DrawFrame()
  {
    if (m_work.NeedRedraw())
    {
      m_work.SetNeedRedraw(false);

      shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_work.GetRenderPolicy()->GetDrawer().get()));

      m_work.BeginPaint(paintEvent);
      m_work.DoPaint(paintEvent);

      NVEventSwapBuffersEGL();

      m_work.EndPaint(paintEvent);
    }
  }

  void Framework::Move(int mode, double x, double y)
  {
    DragEvent e(x, y);
    switch (mode)
    {
    case 0: m_work.StartDrag(e); break;
    case 1: m_work.DoDrag(e); break;
    case 2: m_work.StopDrag(e); break;
    }
  }

  void Framework::Zoom(int mode, double x1, double y1, double x2, double y2)
  {
    ScaleEvent e(x1, y1, x2, y2);
    switch (mode)
    {
    case 0: m_work.StartScale(e); break;
    case 1: m_work.DoScale(e); break;
    case 2: m_work.StopScale(e); break;
    }
  }

  void Framework::StartTouchTask(double x, double y, unsigned ms)
  {
    KillTouchTask();

    m_scheduledTask.reset(new ScheduledTask(bind(&android::Framework::OnProcessTouchTask, this, x, y, ms), ms));
  }

  void Framework::KillTouchTask()
  {
    if (m_scheduledTask)
    {
      m_scheduledTask->Cancel();
      m_scheduledTask.reset();
    }
  }

  /// @param[in] mask Active pointers bits : 0x0 - no, 0x1 - (x1, y1), 0x2 - (x2, y2), 0x3 - (x1, y1)(x2, y2).
  void Framework::Touch(int action, int mask, double x1, double y1, double x2, double y2)
  {
    NVMultiTouchEventType eventType = static_cast<NVMultiTouchEventType>(action);

    // Check if we touch is canceled or we get coordinates NOT from the first pointer.
    if ((mask != 0x1) || (eventType == NV_MULTITOUCH_CANCEL))
    {
      if (mask == 0x1)
        m_work.GetGuiController()->OnTapCancelled(m2::PointD(x1, y1));

      m_isCleanSingleClick = false;
      KillTouchTask();
    }
    else
    {
      ASSERT_EQUAL(mask, 0x1, ());

      if (eventType == NV_MULTITOUCH_DOWN)
      {
        KillTouchTask();

        m_wasLongClick = false;
        m_isCleanSingleClick = true;
        m_lastX1 = x1;
        m_lastY1 = y1;

        if (m_work.GetGuiController()->OnTapStarted(m2::PointD(x1, y1)))
          return;

        StartTouchTask(x1, y1, LONG_TOUCH_MS);
      }

      if (eventType == NV_MULTITOUCH_MOVE)
      {
        double const minDist = m_work.GetVisualScale() * 10.0;
        if ((fabs(x1 - m_lastX1) > minDist) || (fabs(y1 - m_lastY1) > minDist))
        {
          m_isCleanSingleClick = false;
          KillTouchTask();
        }

        if (m_work.GetGuiController()->OnTapMoved(m2::PointD(x1, y1)))
          return;
      }

      if (eventType == NV_MULTITOUCH_UP)
      {
        KillTouchTask();

        if (m_work.GetGuiController()->OnTapEnded(m2::PointD(x1, y1)))
          return;

        if (!m_wasLongClick && m_isCleanSingleClick)
        {
          if (m_doubleClickTimer.ElapsedSeconds() <= DOUBLE_TOUCH_S)
          {
            // performing double-click
            m_work.ScaleToPoint(ScaleToPointEvent(x1, y1, 1.5));
          }
          else
          {
            // starting single touch task
            StartTouchTask(x1, y1, SHORT_TOUCH_MS);

            // starting double click
            m_doubleClickTimer.Reset();
          }
        }
        else
          m_wasLongClick = false;
      }
    }

    // general case processing
    if (m_mask != mask)
    {
      if (m_mask == 0x0)
      {
        if (mask == 0x1)
          m_work.StartDrag(DragEvent(x1, y1));

        if (mask == 0x2)
          m_work.StartDrag(DragEvent(x2, y2));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x1)
      {
        m_work.StopDrag(DragEvent(x1, y1));

        if (mask == 0x0)
        {
          if ((eventType != NV_MULTITOUCH_UP) && (eventType != NV_MULTITOUCH_CANCEL))
            LOG(LWARNING, ("should be NV_MULTITOUCH_UP or NV_MULTITOUCH_CANCEL"));
        }

        if (m_mask == 0x2)
          m_work.StartDrag(DragEvent(x2, y2));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x2)
      {
        m_work.StopDrag(DragEvent(x2, y2));

        if (mask == 0x0)
        {
          if ((eventType != NV_MULTITOUCH_UP) && (eventType != NV_MULTITOUCH_CANCEL))
            LOG(LWARNING, ("should be NV_MULTITOUCH_UP or NV_MULTITOUCH_CANCEL"));
        }

        if (mask == 0x1)
          m_work.StartDrag(DragEvent(x1, y1));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x3)
      {
        m_work.StopScale(ScaleEvent(m_x1, m_y1, m_x2, m_y2));

        if ((eventType == NV_MULTITOUCH_MOVE))
        {
          if (mask == 0x1)
            m_work.StartDrag(DragEvent(x1, y1));

          if (mask == 0x2)
            m_work.StartDrag(DragEvent(x2, y2));
        }
        else
          mask = 0;
      }
    }
    else
    {
      if (eventType == NV_MULTITOUCH_MOVE)
      {
        if (m_mask == 0x1)
          m_work.DoDrag(DragEvent(x1, y1));
        if (m_mask == 0x2)
          m_work.DoDrag(DragEvent(x2, y2));
        if (m_mask == 0x3)
          m_work.DoScale(ScaleEvent(x1, y1, x2, y2));
      }

      if ((eventType == NV_MULTITOUCH_CANCEL) || (eventType == NV_MULTITOUCH_UP))
      {
        if (m_mask == 0x1)
          m_work.StopDrag(DragEvent(x1, y1));
        if (m_mask == 0x2)
          m_work.StopDrag(DragEvent(x2, y2));
        if (m_mask == 0x3)
          m_work.StopScale(ScaleEvent(m_x1, m_y1, m_x2, m_y2));
        mask = 0;
      }
    }

    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;
    m_mask = mask;
  }

  void Framework::ShowSearchResult(search::Result const & r)
  {
    m_doLoadState = false;

    ::Framework::AddressInfo info;
    info.MakeFrom(r);
    ActivatePopupWithAddressInfo(r.GetFeatureCenter(), info);

    m_work.ShowSearchResult(r);
  }

  bool Framework::Search(search::SearchParams const & params)
  {
    m_searchQuery = params.m_query;
    return m_work.Search(params);
  }

  void Framework::LoadState()
  {
    if (!m_work.LoadState())
      m_work.ShowAll();
  }

  void Framework::SaveState()
  {
    m_work.SaveState();
  }

  void Framework::Invalidate()
  {
    m_work.Invalidate();
  }

  void Framework::SetupMeasurementSystem()
  {
    m_work.SetupMeasurementSystem();
  }

  void Framework::AddLocalMaps()
  {
    m_work.AddLocalMaps();
  }

  void Framework::RemoveLocalMaps()
  {
    m_work.RemoveLocalMaps();
  }

  void Framework::GetMapsWithoutSearch(vector<string> & out) const
  {
    ASSERT ( out.empty(), () );

    Platform const & pl = GetPlatform();

    vector<string> v;
    m_work.GetLocalMaps(v);

    for (size_t i = 0; i < v.size(); ++i)
    {
      // skip World and WorldCoast
      if (v[i].find(WORLD_FILE_NAME) == string::npos &&
          v[i].find(WORLD_COASTS_FILE_NAME) == string::npos)
      {
        try
        {
          FilesContainerR cont(pl.GetReader(v[i]));
          if (!cont.IsReaderExist(SEARCH_INDEX_FILE_TAG))
          {
            my::GetNameWithoutExt(v[i]);
            out.push_back(v[i]);
          }
        }
        catch (RootException const & ex)
        {
          // sdcard can contain dummy _*.mwm files. Supress this errors.
          LOG(LWARNING, ("Bad mwm file:", v[i], "Error:", ex.Msg()));
        }
      }
    }
  }

  storage::TIndex Framework::GetCountryIndex(double lat, double lon) const
  {
    return m_work.GetCountryIndex(m2::PointD(MercatorBounds::LonToX(lon),
                                             MercatorBounds::LatToY(lat)));
  }

  string Framework::GetCountryCode(double lat, double lon) const
  {
    return m_work.GetCountryCode(m2::PointD(MercatorBounds::LonToX(lon),
                                            MercatorBounds::LatToY(lat)));
  }

  string Framework::GetCountryNameIfAbsent(m2::PointD const & pt) const
  {
    using namespace storage;

    TIndex const idx = m_work.GetCountryIndex(pt);
    TStatus const status = m_work.GetCountryStatus(idx);
    if (status != EOnDisk && status != EOnDiskOutOfDate)
      return m_work.GetCountryName(idx);
    else
      return string();
  }

  m2::PointD Framework::GetViewportCenter() const
  {
    return m_work.GetViewportCenter();
  }

  void Framework::AddString(string const & name, string const & value)
  {
    m_work.AddString(name, value);
  }

  void Framework::Scale(double k)
  {
    m_work.Scale(k);
  }

  ::Framework * Framework::NativeFramework()
  {
    return &m_work;
  }

  void Framework::OnProcessTouchTask(double x, double y, unsigned ms)
  {
    m2::PointD pt(x, y);
    ::Framework::AddressInfo addrInfo;
    m2::PointD pxPivot;
    BookmarkAndCategory bmAndCat;
    switch (m_work.GetBookmarkOrPoi(pt, pxPivot, addrInfo, bmAndCat))
    {
    case ::Framework::BOOKMARK:
      {
        Bookmark const * pBM = m_work.GetBmCategory(bmAndCat.first)->GetBookmark(bmAndCat.second);
        ActivatePopup(pBM->GetOrg(), pBM->GetName(), IMAGE_ARROW);
        return;
      }
    case ::Framework::POI:
      if (!GetBookmarkBalloon()->isVisible())
      {
        ActivatePopupWithAddressInfo(m_work.PtoG(pxPivot), addrInfo);
        if (ms == LONG_TOUCH_MS)
          m_wasLongClick = true;
        return;
      }
      /* no break*/
    default:
      if (ms == LONG_TOUCH_MS)
      {
        m_work.GetAddressInfo(pt, addrInfo);
        ActivatePopupWithAddressInfo(m_work.PtoG(pt), addrInfo);
        m_wasLongClick = true;
        return;
      }
      /* no break*/
    }

    DeactivatePopup();
  }

  void Framework::ActivatePopupWithAddressInfo(m2::PointD const & pos, ::Framework::AddressInfo const & addrInfo)
  {
    string name = addrInfo.FormatPinText();
    if (name.empty())
      name = m_work.GetStringsBundle().GetString("dropped_pin");

    ActivatePopup(pos, name, IMAGE_PLUS);

    m_work.DrawPlacemark(pos);
    m_work.Invalidate();
  }

  void Framework::ActivatePopup(m2::PointD const & pos, string const & name, PopupImageIndexT index)
  {
    BookmarkBalloon * b = GetBookmarkBalloon();

    m_work.DisablePlacemark();

    b->setImage(*m_images[index]);
    b->setGlbPivot(pos);
    b->setBookmarkName(name);
    b->setIsVisible(true);

    m_work.Invalidate();
  }

  void Framework::DeactivatePopup()
  {
    BookmarkBalloon * b = GetBookmarkBalloon();
    b->setIsVisible(false);

    m_work.DisablePlacemark();
    m_work.Invalidate();
  }

  void Framework::OnBalloonClick(gui::Element * e)
  {
    BookmarkBalloon * balloon = GetBookmarkBalloon();

    BookmarkAndCategory bac;
    if (GetPlatform().IsPro())
    {
      bac = m_work.GetBookmark(m_work.GtoP(balloon->glbPivot()));
      if (!IsValid(bac))
      {
        // add new bookmark
        Bookmark bm(balloon->glbPivot(), balloon->bookmarkName(), m_bmType);
        bac = AddBookmark(m_work.LastEditedCategory(), bm);
      }
    }

    // start edit an existing bookmark
    m_balloonClickListener(bac);
  }

  void Framework::AddBalloonClickListener(TOnBalloonClickListener const & l)
  {
    m_balloonClickListener = l;
  }

  void Framework::RemoveBalloonClickListener()
  {
    m_balloonClickListener.clear();
  }

  void Framework::CreateBookmarkBalloon()
  {
    CHECK(m_work.GetGuiController(), ());

    BookmarkBalloon::Params bp;

    bp.m_position = graphics::EPosAbove;
    bp.m_depth = graphics::maxDepth;
    bp.m_pivot = m2::PointD(0, 0);
    bp.m_imageMarginBottom = 10;
    bp.m_imageMarginLeft = 0;
    bp.m_imageMarginRight = 10;
    bp.m_imageMarginTop = 7;
    bp.m_textMarginBottom = 10;
    bp.m_textMarginLeft = 10;
    bp.m_textMarginRight = 10;
    bp.m_textMarginTop = 7;
    bp.m_text = "Bookmark";
    bp.m_framework = &m_work;

    m_bmBaloon.reset(new BookmarkBalloon(bp));
    m_bmBaloon->setIsVisible(false);
    m_bmBaloon->setOnClickListener(bind(&Framework::OnBalloonClick, this, _1));
    m_work.GetGuiController()->AddElement(m_bmBaloon);
  }

  BookmarkBalloon * Framework::GetBookmarkBalloon()
  {
    if (!m_bmBaloon)
      CreateBookmarkBalloon();
    return m_bmBaloon.get();
  }

  BookmarkAndCategory Framework::AddBookmark(size_t cat, Bookmark & bm)
  {
    size_t const ind = m_work.AddBookmark(cat, bm);

    BookmarkCategory * pCat = m_work.GetBmCategory(cat);
    pCat->SetVisible(true);
    pCat->SaveToKMLFile();

    return BookmarkAndCategory(cat, ind);
  }

  void Framework::ReplaceBookmark(BookmarkAndCategory const & ind, Bookmark & bm)
  {
    m_bmType = bm.GetType();

    BookmarkCategory * pCat = m_work.GetBmCategory(ind.first);
    pCat->ReplaceBookmark(ind.second, bm);
    pCat->SaveToKMLFile();
  }

  size_t Framework::ChangeBookmarkCategory(BookmarkAndCategory const & ind, size_t newCat)
  {
    BookmarkCategory * pOld = m_work.GetBmCategory(ind.first);

    Bookmark bmk(*(pOld->GetBookmark(ind.second)));

    pOld->DeleteBookmark(ind.second);
    pOld->SaveToKMLFile();

    return AddBookmark(newCat, bmk).second;
  }

  bool Framework::IsDownloadingActive()
  {
    return m_work.Storage().IsDownloadInProgress();
  }

  bool Framework::SetViewportByUrl(string const & url)
  {
    //TODO this is weird hack, we should reconsider Android
    // lifecycle handling design
    m_doLoadState = false;
    url_api:: Request request;
    bool success = m_work.SetViewportByURL(url, request);
    // Show temp balloon
    if (success && !request.m_points.empty())
    {
      m2::PointD pt(MercatorBounds::LonToX(request.m_viewportLon),
                    MercatorBounds::LatToY(request.m_viewportLat));

      ActivatePopup(pt, request.m_points.front().m_name, IMAGE_PLUS);
      m_work.DrawPlacemark(pt);
      m_work.Invalidate();

      return true;
    }
    return false;
  }
}
