#include "Framework.hpp"
#include "VideoTimer.hpp"
#include "MapStorage.hpp"

#include "../core/jni_helper.hpp"
#include "../core/render_context.hpp"

#include "../country/country_helper.hpp"

#include "../platform/Platform.hpp"

#include "map/framework.hpp"
#include "map/user_mark.hpp"

#include "gui/controller.hpp"

#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/opengl.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "geometry/angles.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/location.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "storage/storage_defines.hpp"

#include "base/math.hpp"
#include "base/logging.hpp"

namespace
{
const unsigned LONG_TOUCH_MS = 1000;
const unsigned SHORT_TOUCH_MS = 250;
const double DOUBLE_TOUCH_S = SHORT_TOUCH_MS / 1000.0;
}

android::Framework * g_framework = 0;

using namespace storage;

namespace
{

::Framework * frm()
{
  return g_framework->NativeFramework();
}

}

namespace android
{
  void ShowAllSearchResultsImpl()
  {
    frm()->ShowAllSearchResults();
  }

  void Framework::CallRepaint() {}

  Framework::Framework()
   : m_mask(0),
     m_isCleanSingleClick(false),
     m_doLoadState(true),
     m_lastCompass(0.0),
     m_wasLongClick(false),
     m_densityDpi(0),
     m_screenWidth(0),
     m_screenHeight(0)
  {
    ASSERT_EQUAL ( g_framework, 0, () );
    g_framework = this;

    m_videoTimer = new VideoTimer(bind(&Framework::CallRepaint, this));
    m_activeMapsConnectionID = m_work.GetCountryTree().GetActiveMapLayout().AddListener(this);
  }

  Framework::~Framework()
  {
    m_work.GetCountryTree().GetActiveMapLayout().RemoveListener(m_activeMapsConnectionID);
    delete m_videoTimer;
  }

  void Framework::OnLocationError(int errorCode)
  {
    m_work.OnLocationError(static_cast<location::TLocationError>(errorCode));
  }

  void Framework::OnLocationUpdated(location::GpsInfo const & info)
  {
    Platform::RunOnGuiThreadImpl(bind(&::Framework::OnLocationUpdate, ref(m_work), info));
  }

  void Framework::OnCompassUpdated(location::CompassInfo const & info)
  {
    static double const COMPASS_THRASHOLD = my::DegToRad(1.0);

    /// @todo Do not emit compass bearing too often while we are passing it through nv-queue.
    /// Need to make more experiments in future.
    if (fabs(ang::GetShortestDistance(m_lastCompass, info.m_bearing)) >= COMPASS_THRASHOLD)
    {
      m_lastCompass = info.m_bearing;
      Platform::RunOnGuiThreadImpl(bind(&::Framework::OnCompassUpdate, ref(m_work), info));
    }
  }

  void Framework::UpdateCompassSensor(int ind, float * arr)
  {
    m_sensors[ind].Next(arr);
  }

  void Framework::DeleteRenderPolicy()
  {
    m_work.SaveState();
    LOG(LINFO, ("Clearing current render policy."));
    m_work.SetRenderPolicy(nullptr);
    m_work.EnterBackground();
  }

  void Framework::SetBestDensity(int densityDpi, RenderPolicy::Params & params)
  {
    typedef pair<int, graphics::EDensity> P;
    P dens[] = {
        P(120, graphics::EDensityLDPI),
        P(160, graphics::EDensityMDPI),
        P(240, graphics::EDensityHDPI),
        P(320, graphics::EDensityXHDPI),
        P(480, graphics::EDensityXXHDPI)
    };

    int prevRange = numeric_limits<int>::max();
    int bestRangeIndex = 0;
    for (int i = 0; i < ARRAY_SIZE(dens); i++)
    {
      int currRange = abs(densityDpi - dens[i].first);
      if (currRange <= prevRange)
      {
        // it is better, take index
        bestRangeIndex = i;
        prevRange = currRange;
      }
      else
        break;
    }

    params.m_density = dens[bestRangeIndex].second;
  }

  bool Framework::InitRenderPolicyImpl(int densityDpi, int screenWidth, int screenHeight)
  {
    graphics::ResourceManager::Params rmParams;

    rmParams.m_videoMemoryLimit = 30 * 1024 * 1024;
    rmParams.m_texFormat = graphics::Data4Bpp;

    RenderPolicy::Params rpParams;

    rpParams.m_videoTimer = m_videoTimer;
    rpParams.m_useDefaultFB = true;
    rpParams.m_rmParams = rmParams;
    rpParams.m_primaryRC = make_shared<android::RenderContext>();

    SetBestDensity(densityDpi, rpParams);

    rpParams.m_skinName = "basic.skn";
    LOG(LINFO, ("Using", graphics::convert(rpParams.m_density), "resources"));

    rpParams.m_screenWidth = screenWidth;
    rpParams.m_screenHeight = screenHeight;

    try
    {
      m_work.SetRenderPolicy(CreateRenderPolicy(rpParams));
      m_work.InitGuiSubsystem();
    }
    catch (graphics::gl::platform_unsupported const & e)
    {
      LOG(LINFO, ("This android platform is unsupported, reason:", e.what()));
      return false;
    }

    return true;
  }

  bool Framework::InitRenderPolicy(int densityDpi, int screenWidth, int screenHeight)
  {
    if (!InitRenderPolicyImpl(densityDpi, screenWidth, screenHeight))
      return false;

    if (m_doLoadState)
      LoadState();
    else
      m_doLoadState = true;

    m_work.SetUpdatesEnabled(true);
    m_work.EnterForeground();

    m_densityDpi = densityDpi;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    return true;
  }

  void Framework::SetMapStyle(MapStyle mapStyle)
  {
    if (m_work.GetMapStyle() == mapStyle)
      return;

    // drop old render policy
    m_work.SetRenderPolicy(nullptr);

    m_work.SetMapStyle(mapStyle);

    // construct new render policy
    if (!InitRenderPolicyImpl(m_densityDpi, m_screenWidth, m_screenHeight))
      return;

    m_work.SetUpdatesEnabled(true);
  }

  Storage & Framework::Storage()
  {
    return m_work.Storage();
  }

  CountryStatusDisplay * Framework::GetCountryStatusDisplay()
  {
    return m_work.GetCountryStatusDisplay();
  }

  void Framework::ShowCountry(TIndex const & idx, bool zoomToDownloadButton)
  {
    m_doLoadState = false;

    if (zoomToDownloadButton)
    {
        m2::RectD const rect = m_work.GetCountryBounds(idx);
        double const lon = MercatorBounds::XToLon(rect.Center().x);
        double const lat = MercatorBounds::YToLat(rect.Center().y);
        m_work.ShowRect(lat, lon, 10);
    }
    else
      m_work.ShowCountry(idx);
  }

  TStatus Framework::GetCountryStatus(TIndex const & idx) const
  {
    return m_work.GetCountryStatus(idx);
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
    if (KillTouchTask())
      m_scheduledTask.reset(new ScheduledTask(bind(&android::Framework::OnProcessTouchTask, this, x, y, ms), ms));
  }

  bool Framework::KillTouchTask()
  {
    if (m_scheduledTask)
    {
      if (!m_scheduledTask->CancelNoBlocking())
      {
        // The task is already running - skip new task.
        return false;
      }

      m_scheduledTask.reset();
    }
    return true;
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

        if (eventType == NV_MULTITOUCH_MOVE)
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
    m_work.ShowSearchResult(r);
  }

  void Framework::ShowAllSearchResults()
  {
    m_doLoadState = false;
    Platform::RunOnGuiThreadImpl(bind(&ShowAllSearchResultsImpl));
  }

  /*
  void Framework::CleanSearchLayerOnMap()
  {
    ::Framework * f = NativeFramework();

    // Call ClearXXX first, then RemovePin (Framework::Invalidate is called inside).
    f->GetBookmarkManager().UserMarksClear(UserMarkContainer::SEARCH_MARK);
    f->GetBalloonManager().RemovePin();
    f->GetBalloonManager().Dismiss();
    f->Invalidate();
  }
  */

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
    m_work.RegisterAllMaps();
  }

  void Framework::RemoveLocalMaps()
  {
    m_work.DeregisterAllMaps();
  }

  void Framework::GetMapsWithoutSearch(vector<string> & out) const
  {
    ASSERT ( out.empty(), () );

    ::Platform const & pl = GetPlatform();

    vector<string> v;
    m_work.GetMaps(v);

    for (size_t i = 0; i < v.size(); ++i)
    {
      // skip World and WorldCoast
      if (v[i].find(WORLD_FILE_NAME) == string::npos &&
          v[i].find(WORLD_COASTS_FILE_NAME) == string::npos)
      {
        try
        {
          FilesContainerR cont(pl.GetReader(v[i]));
          if (!cont.IsExist(SEARCH_INDEX_FILE_TAG))
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

  TIndex Framework::GetCountryIndex(double lat, double lon) const
  {
    return m_work.GetCountryIndex(MercatorBounds::FromLatLon(lat, lon));
  }

  string Framework::GetCountryCode(double lat, double lon) const
  {
    return m_work.GetCountryCode(MercatorBounds::FromLatLon(lat, lon));
  }

  string Framework::GetCountryNameIfAbsent(m2::PointD const & pt) const
  {
    TIndex const idx = m_work.GetCountryIndex(pt);
    TStatus const status = m_work.GetCountryStatus(idx);
    if (status != TStatus::EOnDisk && status != TStatus::EOnDiskOutOfDate)
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
    m_wasLongClick = (ms == LONG_TOUCH_MS);
    GetPinClickManager().OnShowMark(m_work.GetUserMark(m2::PointD(x, y), m_wasLongClick));
  }

  BookmarkAndCategory Framework::AddBookmark(size_t cat, m2::PointD const & pt, BookmarkData & bm)
  {
    return BookmarkAndCategory(cat, m_work.AddBookmark(cat, pt, bm));
  }

  void Framework::ReplaceBookmark(BookmarkAndCategory const & ind, BookmarkData & bm)
  {
    m_work.ReplaceBookmark(ind.first, ind.second, bm);
  }

  size_t Framework::ChangeBookmarkCategory(BookmarkAndCategory const & ind, size_t newCat)
  {
    BookmarkCategory * pOld = m_work.GetBmCategory(ind.first);
    Bookmark const * oldBm = pOld->GetBookmark(ind.second);
    m2::PointD pt = oldBm->GetOrg();
    BookmarkData bm(oldBm->GetName(), oldBm->GetType(), oldBm->GetDescription(),
                    oldBm->GetScale(), oldBm->GetTimeStamp());

    pOld->DeleteBookmark(ind.second);
    pOld->SaveToKMLFile();

    return AddBookmark(newCat, pt, bm).second;
  }

  bool Framework::IsDownloadingActive()
  {
    return m_work.Storage().IsDownloadInProgress();
  }

  bool Framework::ShowMapForURL(string const & url)
  {
    /// @todo this is weird hack, we should reconsider Android lifecycle handling design
    m_doLoadState = false;

    return m_work.ShowMapForURL(url);
  }

  void Framework::DeactivatePopup()
  {
    GetPinClickManager().RemovePin();
  }

  string Framework::GetOutdatedCountriesString()
  {
    vector<Country const *> countries;
    Storage().GetOutdatedCountries(countries);

    string res;
    for (size_t i = 0; i < countries.size(); ++i)
    {
      res += countries[i]->Name();
      if (i < countries.size() - 1)
        res += ", ";
    }
    return res;
  }

  void Framework::ShowTrack(int category, int track)
  {
    Track const * nTrack = NativeFramework()->GetBmCategory(category)->GetTrack(track);
    m_doLoadState = false;
    NativeFramework()->ShowTrack(*nTrack);
  }

  void Framework::SetCountryTreeListener(shared_ptr<jobject> objPtr)
  {
    m_javaCountryListener = objPtr;
    m_work.GetCountryTree().SetListener(this);
  }

  void Framework::ResetCountryTreeListener()
  {
    m_work.GetCountryTree().ResetListener();
    m_javaCountryListener.reset();
  }

  int Framework::AddActiveMapsListener(shared_ptr<jobject> obj)
  {
    m_javaActiveMapListeners[m_currentSlotID] = obj;
    return m_currentSlotID++;
  }

  void Framework::RemoveActiveMapsListener(int slotID)
  {
    m_javaActiveMapListeners.erase(slotID);
  }
  //////////////////////////////////////////////////////////////////////////////////////////
  void Framework::ItemStatusChanged(int childPosition)
  {
    if (m_javaCountryListener == NULL)
      return;

    JNIEnv * env = jni::GetEnv();
    jmethodID const methodID = jni::GetJavaMethodID(env,
                                                    *m_javaCountryListener,
                                                    "onItemStatusChanged",
                                                    "(I)V");
    ASSERT ( methodID, () );

    env->CallVoidMethod(*m_javaCountryListener, methodID, childPosition);
  }

  void Framework::ItemProgressChanged(int childPosition, LocalAndRemoteSizeT const & sizes)
  {
    if (m_javaCountryListener == NULL)
      return;

    JNIEnv * env = jni::GetEnv();
    jmethodID const methodID = jni::GetJavaMethodID(env,
                                                    *m_javaCountryListener,
                                                    "onItemProgressChanged",
                                                    "(I[J)V");
    ASSERT ( methodID, () );

    env->CallVoidMethod(*m_javaCountryListener, methodID, childPosition, storage_utils::ToArray(env, sizes));
  }

  void Framework::CountryGroupChanged(ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                                      ActiveMapsLayout::TGroup const & newGroup, int newPosition)
  {
    JNIEnv * env = jni::GetEnv();
    for (TListenerMap::const_iterator it = m_javaActiveMapListeners.begin(); it != m_javaActiveMapListeners.end(); ++it)
    {
      jmethodID const methodID = jni::GetJavaMethodID(env, *(it->second), "onCountryGroupChanged", "(IIII)V");
      ASSERT ( methodID, () );

      env->CallVoidMethod(*(it->second), methodID, oldGroup, oldPosition, newGroup, newPosition);
    }
  }

  void Framework::CountryStatusChanged(ActiveMapsLayout::TGroup const & group, int position,
                                       TStatus const & oldStatus, TStatus const & newStatus)
  {
    JNIEnv * env = jni::GetEnv();
    for (TListenerMap::const_iterator it = m_javaActiveMapListeners.begin(); it != m_javaActiveMapListeners.end(); ++it)
    {
      jmethodID const methodID = jni::GetJavaMethodID(env, *(it->second), "onCountryStatusChanged", "(IIII)V");
      ASSERT ( methodID, () );

      env->CallVoidMethod(*(it->second), methodID, group, position,
                             static_cast<jint>(oldStatus), static_cast<jint>(newStatus));
    }
  }

  void Framework::CountryOptionsChanged(ActiveMapsLayout::TGroup const & group, int position,
                                        TMapOptions const & oldOpt, TMapOptions const & newOpt)
  {
    JNIEnv * env = jni::GetEnv();
    for (TListenerMap::const_iterator it = m_javaActiveMapListeners.begin(); it != m_javaActiveMapListeners.end(); ++it)
    {
      jmethodID const methodID = jni::GetJavaMethodID(env, *(it->second), "onCountryOptionsChanged", "(IIII)V");
      ASSERT ( methodID, () );

      env->CallVoidMethod(*(it->second), methodID, group, position,
                              static_cast<jint>(oldOpt), static_cast<jint>(newOpt));
    }
  }

  void Framework::DownloadingProgressUpdate(ActiveMapsLayout::TGroup const & group, int position,
                                            LocalAndRemoteSizeT const & progress)
  {
    JNIEnv * env = jni::GetEnv();
    for (TListenerMap::const_iterator it = m_javaActiveMapListeners.begin(); it != m_javaActiveMapListeners.end(); ++it)
    {
      jmethodID const methodID = jni::GetJavaMethodID(env, *(it->second), "onCountryProgressChanged", "(II[J)V");
      ASSERT ( methodID, () );

      env->CallVoidMethod(*(it->second), methodID, group, position, storage_utils::ToArray(env, progress));
    }
  }

  // Fills mapobject's metadata from UserMark
  void Framework::InjectMetadata(JNIEnv * env, jclass const clazz, jobject const mapObject, UserMark const * userMark)
  {
    feature::FeatureMetadata metadata;
    frm()->FindClosestPOIMetadata(userMark->GetOrg(), metadata);

    static jmethodID const addId = env->GetMethodID(clazz, "addMetadata", "(ILjava/lang/String;)V");
    ASSERT ( addId, () );

    const vector<feature::FeatureMetadata::EMetadataType> metaTypes = metadata.GetPresentTypes();
    for (size_t i = 0; i < metaTypes.size(); i++)
    {
      feature::FeatureMetadata::EMetadataType metaType = static_cast<feature::FeatureMetadata::EMetadataType>(metaTypes[i]);
      jstring metaString = jni::ToJavaString(env, metadata.Get(metaType));
      env->CallVoidMethod(mapObject, addId, metaType, metaString);
      // TODO use unique_ptrs for autoallocation of local refs
      env->DeleteLocalRef(metaString);
    }
  }
}

template <class T>
T const * CastMark(UserMark const * data)
{
  return static_cast<T const *>(data);
}


//============ GLUE CODE for com.mapswithme.maps.Framework class =============//
/*            ____
 *          _ |||| _
 *          \\    //
 *           \\  //
 *            \\//
 *             \/
 */

extern "C"
{
  pair<jintArray, jobjectArray> NativeMetadataToJavaMetadata(JNIEnv * env, feature::FeatureMetadata const & metadata)
  {
    const vector<feature::FeatureMetadata::EMetadataType> metaTypes = metadata.GetPresentTypes();
    // FIXME arrays, allocated through New<Type>Array should be deleted manually in the method.
    // refactor that to delete refs locally or pass arrays from outside context
    const jintArray j_metaTypes = env->NewIntArray(metadata.Size());
    jint * arr = env->GetIntArrayElements(j_metaTypes, 0);
    const jobjectArray j_metaValues = env->NewObjectArray(metadata.Size(), jni::GetStringClass(env), 0);

    for (size_t i = 0; i < metaTypes.size(); i++)
    {
      arr[i] = metaTypes[i];
      feature::FeatureMetadata::EMetadataType metaType = static_cast<feature::FeatureMetadata::EMetadataType>(metaTypes[i]);
      jstring metaString = jni::ToJavaString(env, metadata.Get(metaType));
      env->SetObjectArrayElement(j_metaValues, i, metaString);
      env->DeleteLocalRef(metaString);
    }
    env->ReleaseIntArrayElements(j_metaTypes, arr, 0);

    return make_pair(j_metaTypes, j_metaValues);
  }

  // API
  void CallOnApiPointActivatedListener(shared_ptr<jobject> obj, ApiMarkPoint const * data, double lat, double lon)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const methodID = jni::GetJavaMethodID(env,
                                                    *obj.get(),
                                                   "onApiPointActivated",
                                                   "(DDLjava/lang/String;Ljava/lang/String;)V");

    jstring j_name = jni::ToJavaString(env, data->GetName());
    jstring j_id = jni::ToJavaString(env, data->GetID());

    env->CallVoidMethod(*obj.get(), methodID, lat, lon, j_name, j_id);

    // TODO use unique_ptrs for autoallocation of local refs
    env->DeleteLocalRef(j_id);
    env->DeleteLocalRef(j_name);
  }

  // Additional layer
  void CallOnAdditionalLayerActivatedListener(shared_ptr<jobject> obj, m2::PointD const & globalPoint,
      search::AddressInfo const & addrInfo, feature::FeatureMetadata const & metadata)
  {
    JNIEnv * env = jni::GetEnv();

    const jstring j_name = jni::ToJavaString(env, addrInfo.GetPinName());
    const jstring j_type = jni::ToJavaString(env, addrInfo.GetPinType());
    const double lon = MercatorBounds::XToLon(globalPoint.x);
    const double lat = MercatorBounds::YToLat(globalPoint.y);

    pair<jintArray, jobjectArray> const meta = NativeMetadataToJavaMetadata(env, metadata);

    const char * signature = "(Ljava/lang/String;Ljava/lang/String;DD[I[Ljava/lang/String;)V";
    static jmethodID const methodId = jni::GetJavaMethodID(env, *obj.get(),
                                                      "onAdditionalLayerActivated", signature);
    env->CallVoidMethod(*obj.get(), methodId, j_name, j_type, lat, lon, meta.first, meta.second);

    env->DeleteLocalRef(j_type);
    env->DeleteLocalRef(j_name);

    env->DeleteLocalRef(meta.second);
    env->DeleteLocalRef(meta.first);
  }

  // POI
  void CallOnPoiActivatedListener(shared_ptr<jobject> obj, m2::PointD const & globalPoint,
      search::AddressInfo const & addrInfo, feature::FeatureMetadata const & metadata)
  {
    JNIEnv * env = jni::GetEnv();

    const jstring j_name = jni::ToJavaString(env, addrInfo.GetPinName());
    const jstring j_type = jni::ToJavaString(env, addrInfo.GetPinType());
    const jstring j_address = jni::ToJavaString(env, addrInfo.FormatAddress());
    const double lon = MercatorBounds::XToLon(globalPoint.x);
    const double lat = MercatorBounds::YToLat(globalPoint.y);

    pair<jintArray, jobjectArray> const meta = NativeMetadataToJavaMetadata(env, metadata);

    const char * signature = "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DD[I[Ljava/lang/String;)V";
    static jmethodID const methodId = jni::GetJavaMethodID(env, *obj.get(),
                                                      "onPoiActivated", signature);
    env->CallVoidMethod(*obj.get(), methodId, j_name, j_type, j_address, lat, lon, meta.first, meta.second);

    env->DeleteLocalRef(meta.second);
    env->DeleteLocalRef(meta.first);
  }

  // Bookmark
  void CallOnBookmarkActivatedListener(shared_ptr<jobject> obj, BookmarkAndCategory const & bmkAndCat)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const methodId = jni::GetJavaMethodID(env, *obj.get(),
                                                    "onBookmarkActivated", "(II)V");
    env->CallVoidMethod(*obj.get(), methodId, bmkAndCat.first, bmkAndCat.second);
  }

  // My position
  void CallOnMyPositionActivatedListener(shared_ptr<jobject> obj, double lat, double lon)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const methodId = jni::GetJavaMethodID(env, *obj.get(),
                                                    "onMyPositionActivated", "(DD)V");
    env->CallVoidMethod(*obj.get(), methodId, lat, lon);
  }

  void CallOnUserMarkActivated(shared_ptr<jobject> obj, unique_ptr<UserMarkCopy> markCopy)
  {
    ::Framework * fm = frm();
    UserMark const * mark = markCopy->GetUserMark();
    fm->ActivateUserMark(mark);

    switch (mark->GetMarkType())
    {
    case UserMark::Type::API:
      {
        double lat, lon;
        mark->GetLatLon(lat, lon);
        CallOnApiPointActivatedListener(obj, CastMark<ApiMarkPoint>(mark), lat, lon);
        break;
      }

    case UserMark::Type::BOOKMARK:
      {
        BookmarkAndCategory bmAndCat = fm->FindBookmark(mark);
        if (IsValid(bmAndCat))
          CallOnBookmarkActivatedListener(obj, bmAndCat);
        break;
      }

    case UserMark::Type::POI:
      {
        PoiMarkPoint const * poiMark = CastMark<PoiMarkPoint>(mark);
        CallOnPoiActivatedListener(obj, mark->GetOrg(), poiMark->GetInfo(), poiMark->GetMetadata());
        break;
      }

    case UserMark::Type::SEARCH:
      {
        SearchMarkPoint const * searchMark = CastMark<SearchMarkPoint>(mark);
        feature::FeatureMetadata metadata;
        fm->FindClosestPOIMetadata(mark->GetOrg(), metadata);
        CallOnAdditionalLayerActivatedListener(obj, searchMark->GetOrg(), searchMark->GetInfo(), metadata);
        break;
      }

    case UserMark::Type::MY_POSITION:
      {
        double lat, lon;
        mark->GetLatLon(lat, lon);
        CallOnMyPositionActivatedListener(obj, lat, lon);
        break;
      }
    }
  }

  // Dismiss information box
  void CallOnDismissListener(shared_ptr<jobject> obj)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const methodId = jni::GetJavaMethodID(env, *obj.get(), "onDismiss", "()V");
    ASSERT(methodId, ());
    env->CallVoidMethod(*obj.get(), methodId);
  }

  void CallRoutingListener(shared_ptr<jobject> obj, int errorCode, vector<storage::TIndex> const & absentCountries)
  {
    JNIEnv * env = jni::GetEnv();
    // cache methodID - it cannot change after class is loaded.
    // http://developer.android.com/training/articles/perf-jni.html#jclass_jmethodID_and_jfieldID more details here
    static jmethodID const methodId = jni::GetJavaMethodID(env, *obj.get(), "onRoutingEvent", "(I[Lcom/mapswithme/maps/MapStorage$Index;)V");
    ASSERT(methodId, ());

    jobjectArray const countriesJava = env->NewObjectArray(absentCountries.size(), g_indexClazz, 0);
    for (size_t i = 0; i < absentCountries.size(); i++)
    {
      jobject country = storage::ToJava(absentCountries[i]);
      env->SetObjectArrayElement(countriesJava, i, country);
      env->DeleteLocalRef(country);
    }

    env->CallVoidMethod(*obj.get(), methodId, errorCode, countriesJava);

    env->DeleteLocalRef(countriesJava);
  }

  /// @name JNI EXPORTS
  //@{
  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetNameAndAddress4Point(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon)
  {
    search::AddressInfo info;

    frm()->GetAddressInfoForGlobalPoint(MercatorBounds::FromLatLon(lat, lon), info);

    return jni::ToJavaString(env, info.FormatNameAndAddress());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeClearApiPoints(JNIEnv * env, jclass clazz)
  {
    frm()->GetBookmarkManager().UserMarksClear(UserMarkContainer::API_MARK);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeConnectBalloonListeners(JNIEnv * env, jclass clazz, jobject l)
  {
    PinClickManager & manager = g_framework->GetPinClickManager();
    shared_ptr<jobject> obj = jni::make_global_ref(l);

    manager.ConnectUserMarkListener(bind(&CallOnUserMarkActivated, obj, _1));
    manager.ConnectDismissListener(bind(&CallOnDismissListener, obj));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeClearBalloonListeners(JNIEnv * env, jobject thiz)
  {
    g_framework->GetPinClickManager().ClearListeners();
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetGe0Url(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jdouble zoomLevel, jstring name)
  {
    ::Framework * fr = frm();
    double const scale = (zoomLevel > 0 ? zoomLevel : fr->GetDrawScale());
    const string url = fr->CodeGe0url(lat, lon, scale, jni::ToNativeString(env, name));
    return jni::ToJavaString(env, url);
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetDistanceAndAzimut(
      JNIEnv * env, jclass clazz, jdouble merX, jdouble merY, jdouble cLat, jdouble cLon, jdouble north)
  {
    string distance;
    double azimut = -1.0;
    frm()->GetDistanceAndAzimut(m2::PointD(merX, merY), cLat, cLon, north, distance, azimut);

    jclass daClazz = env->FindClass("com/mapswithme/maps/bookmarks/data/DistanceAndAzimut");
    ASSERT ( daClazz, () );

    jmethodID methodID = env->GetMethodID(daClazz, "<init>", "(Ljava/lang/String;D)V");
    ASSERT ( methodID, () );

    return env->NewObject(daClazz, methodID,
                          jni::ToJavaString(env, distance.c_str()),
                          static_cast<jdouble>(azimut));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetDistanceAndAzimutFromLatLon(
      JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jdouble cLat, jdouble cLon, jdouble north)
  {
    const double merY = MercatorBounds::LatToY(lat);
    const double merX = MercatorBounds::LonToX(lon);
    return Java_com_mapswithme_maps_Framework_nativeGetDistanceAndAzimut(env, clazz, merX, merY, cLat, cLon, north);
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeFormatLatLon(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jboolean useDMSFormat)
  {
    if (useDMSFormat)
      return jni::ToJavaString(env,  MeasurementUtils::FormatLatLonAsDMS(lat, lon, 2));
    else
      return jni::ToJavaString(env,  MeasurementUtils::FormatLatLon(lat, lon, 6));
  }

  JNIEXPORT jobjectArray JNICALL
  Java_com_mapswithme_maps_Framework_nativeFormatLatLonToArr(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jboolean useDMSFormat)
  {
    string slat, slon;
    if (useDMSFormat)
      MeasurementUtils::FormatLatLonAsDMS(lat, lon, slat, slon, 2);
    else
      MeasurementUtils::FormatLatLon(lat, lon, slat, slon, 6);

    jclass klass = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(2, klass, 0);

    env->SetObjectArrayElement(arr, 0, jni::ToJavaString(env, slat));
    env->SetObjectArrayElement(arr, 1, jni::ToJavaString(env, slon));

    return arr;
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeFormatAltitude(JNIEnv * env, jclass clazz, jdouble alt)
  {
    return jni::ToJavaString(env,  MeasurementUtils::FormatAltitude(alt));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeFormatSpeed(JNIEnv * env, jclass clazz, jdouble speed)
  {
    return jni::ToJavaString(env,  MeasurementUtils::FormatSpeed(speed));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetOutdatedCountriesString(JNIEnv * env, jclass clazz)
  {
    return jni::ToJavaString(env, g_framework->GetOutdatedCountriesString());
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_Framework_nativeIsDataVersionChanged(JNIEnv * env, jclass clazz)
  {
    return frm()->IsDataVersionUpdated() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeUpdateSavedDataVersion(JNIEnv * env, jclass clazz)
  {
    frm()->UpdateSavedDataVersion();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_Framework_getDrawScale(JNIEnv * env, jclass clazz)
  {
    return static_cast<jint>(frm()->GetDrawScale());
  }

  JNIEXPORT jdoubleArray JNICALL
  Java_com_mapswithme_maps_Framework_getScreenRectCenter(JNIEnv * env, jclass clazz)
  {
    const m2::PointD center = frm()->GetViewportCenter();

    double latlon[] = {MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x)};
    jdoubleArray jLatLon = env->NewDoubleArray(2);
    env->SetDoubleArrayRegion(jLatLon, 0, 2, latlon);

    return jLatLon;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeShowTrackRect(JNIEnv * env, jclass clazz, jint cat, jint track)
  {
    g_framework->ShowTrack(cat, track);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_injectData(JNIEnv * env, jclass clazz, jobject jsearchResult, jlong index)
  {
    const size_t nIndex = static_cast<size_t>(index);

    BookmarkManager & m = frm()->GetBookmarkManager();
    UserMarkContainer::Controller & c = m.UserMarksGetController(UserMarkContainer::SEARCH_MARK);
    ASSERT_LESS(nIndex , c.GetUserMarkCount(), ("Invalid index", nIndex));
    UserMark const * mark = c.GetUserMark(nIndex);
    search::AddressInfo const & info= CastMark<SearchMarkPoint>(mark)->GetInfo();

    jclass const javaClazz = env->GetObjectClass(jsearchResult);

    static jfieldID const nameId = env->GetFieldID(javaClazz, "mName", "Ljava/lang/String;");
    env->SetObjectField(jsearchResult, nameId, jni::ToJavaString(env, info.GetPinName()));

    static jfieldID const typeId = env->GetFieldID(javaClazz, "mTypeName", "Ljava/lang/String;");
    env->SetObjectField(jsearchResult, typeId, jni::ToJavaString(env, info.GetPinType()));

    static jfieldID const latId = env->GetFieldID(javaClazz, "mLat", "D");
    env->SetDoubleField(jsearchResult, latId, MercatorBounds::YToLat(mark->GetOrg().y));

    static jfieldID const lonId = env->GetFieldID(javaClazz, "mLon", "D");
    env->SetDoubleField(jsearchResult, lonId, MercatorBounds::XToLon(mark->GetOrg().x));

    g_framework->InjectMetadata(env, javaClazz, jsearchResult, mark);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_cleanSearchLayerOnMap(JNIEnv * env, jclass clazz)
  {
    android::Platform::RunOnGuiThreadImpl(bind(&::Framework::CancelInteractiveSearch, frm()));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_invalidate(JNIEnv * env, jclass clazz)
  {
    g_framework->Invalidate();
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetBookmarkDir(JNIEnv * env, jclass thiz)
  {
    return jni::ToJavaString(env, GetPlatform().SettingsDir().c_str());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetWritableDir(JNIEnv * env, jclass thiz)
  {
    return jni::ToJavaString(env, GetPlatform().WritableDir().c_str());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetSettingsDir(JNIEnv * env, jclass thiz)
  {
    return jni::ToJavaString(env, GetPlatform().SettingsDir().c_str());
  }

  JNIEXPORT jobjectArray JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetMovableFilesExt(JNIEnv * env, jclass thiz)
  {
    jclass stringClass = jni::GetStringClass(env);

    char const * exts[] = { DATA_FILE_EXTENSION, FONT_FILE_EXTENSION, ROUTING_FILE_EXTENSION };
    jobjectArray resultArray = env->NewObjectArray(ARRAY_SIZE(exts), stringClass, NULL);

    for (size_t i = 0; i < ARRAY_SIZE(exts); ++i)
      env->SetObjectArrayElement(resultArray, i, jni::ToJavaString(env, exts[i]));

    return resultArray;
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetBookmarksExt(JNIEnv * env, jclass thiz)
  {
    return jni::ToJavaString(env, BOOKMARKS_FILE_EXTENSION);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeSetWritableDir(JNIEnv * env, jclass thiz, jstring jNewPath)
  {
    string newPath = jni::ToNativeString(env, jNewPath);
    g_framework->RemoveLocalMaps();
    android::Platform::Instance().SetStoragePath(newPath);
    g_framework->AddLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeLoadBookmarks(JNIEnv * env, jclass thiz)
  {
    android::Platform::RunOnGuiThreadImpl(bind(&::Framework::LoadBookmarks, frm()));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_Framework_nativeIsRoutingActive(JNIEnv * env, jclass thiz)
  {
    return frm()->IsRoutingActive();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_Framework_nativeIsRouteBuilt(JNIEnv * env, jclass thiz)
  {
    return frm()->IsRouteBuilt();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeCloseRouting(JNIEnv * env, jclass thiz)
  {
    android::Platform::RunOnGuiThreadImpl(bind(&::Framework::CloseRouting, frm()));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeBuildRoute(JNIEnv * env, jclass thiz, jdouble lat, jdouble lon)
  {
    android::Platform::RunOnGuiThreadImpl(bind(&::Framework::BuildRoute, frm(), MercatorBounds::FromLatLon(lat, lon)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeFollowRoute(JNIEnv * env, jclass thiz)
  {
    android::Platform::RunOnGuiThreadImpl(bind(&::Framework::FollowRoute, frm()));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetRouteFollowingInfo(JNIEnv * env, jclass thiz)
  {
    ::Framework * fr = frm();
    if (!fr->IsRoutingActive())
      return nullptr;

    location::FollowingInfo info;
    fr->GetRouteFollowingInfo(info);
    if (!info.IsValid())
      return nullptr;
    
    jclass const klass = env->FindClass("com/mapswithme/maps/LocationState$RoutingInfo");
    ASSERT(klass, (jni::DescribeException()));
    static jmethodID const ctorRouteInfoID = env->GetMethodID(klass, "<init>",
        "(Ljava/lang/String;Ljava/lang/String;"
        "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II"
        "[Lcom/mapswithme/maps/LocationState$SingleLaneInfo;)V");
    ASSERT(ctorRouteInfoID, (jni::DescribeException()));

    vector<location::FollowingInfo::SingleLaneInfoOuter> const & lanes = info.m_lanes;
    jobjectArray jLanes = nullptr;
    if (!lanes.empty())
    {
      // A new java array of SingleLaneInfo classes for lane information is allocated here.
      // Then it will be saved in com.mapswithme.maps.LocationState, and then removed by java GC.
      jclass const myClassArray = env->FindClass("com/mapswithme/maps/LocationState$SingleLaneInfo");
      ASSERT(myClassArray, (jni::DescribeException()));
      size_t const lanesSize = lanes.size();
      jLanes = env->NewObjectArray(lanesSize, myClassArray, nullptr);
      ASSERT(jLanes, (jni::DescribeException()));
      static jmethodID const ctorSingleLaneInfoID = env->GetMethodID(myClassArray, "<init>", "([BZ)V");
      ASSERT(ctorSingleLaneInfoID, (jni::DescribeException()));

      jbyteArray singleLane = nullptr;
      jobject singleLaneInfo = nullptr;

      for (size_t j = 0; j < lanesSize; ++j)
      {
        size_t const laneSize = lanes[j].m_lane.size();
        singleLane = env->NewByteArray(laneSize);
        ASSERT(singleLane, (jni::DescribeException()));
        env->SetByteArrayRegion(singleLane, 0, laneSize, lanes[j].m_lane.data());
        singleLaneInfo = env->NewObject(myClassArray, ctorSingleLaneInfoID, singleLane, lanes[j].m_isActive);
        ASSERT(singleLaneInfo, (jni::DescribeException()));
        env->SetObjectArrayElement(jLanes, j, singleLaneInfo);
        env->DeleteLocalRef(singleLaneInfo);
        env->DeleteLocalRef(singleLane);
      }
    }

    jobject const result = env->NewObject(
        klass, ctorRouteInfoID, jni::ToJavaString(env, info.m_distToTarget),
        jni::ToJavaString(env, info.m_targetUnitsSuffix),
        jni::ToJavaString(env, info.m_distToTurn),
        jni::ToJavaString(env, info.m_turnUnitsSuffix), jni::ToJavaString(env, info.m_targetName),
        info.m_turn, info.m_time, jLanes);
    ASSERT(result, (jni::DescribeException()));
    return result;
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetMapObjectForPoint(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon)
  {
    PoiMarkPoint const * poiMark = frm()->GetAddressMark(MercatorBounds::FromLatLon(lat, lon));

    jclass klass = env->FindClass("com/mapswithme/maps/bookmarks/data/MapObject$Poi");
    static jmethodID const methodID = env->GetMethodID(klass, "<init>", "(Ljava/lang/String;DDLjava/lang/String;)V");

    jobject const mapObject = env->NewObject(klass, methodID,
                                                        jni::ToJavaString(env, poiMark->GetInfo().GetPinName()),
                                                        lat, lon,
                                                        jni::ToJavaString(env, poiMark->GetInfo().GetPinType()));
    ASSERT(mapObject, ());


    g_framework->InjectMetadata(env, klass, mapObject, poiMark);

    return mapObject;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeActivateUserMark(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon)
  {
    ::Framework * fr = frm();
    m2::PointD pxPoint = fr->GtoP(MercatorBounds::FromLatLon(lat, lon));
    UserMark const * mark = fr->GetUserMark(pxPoint, true);
    fr->GetBalloonManager().OnShowMark(mark);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetCountryNameIfAbsent(JNIEnv * env, jobject thiz,
      jdouble lat, jdouble lon)
  {
    string const name = g_framework->GetCountryNameIfAbsent(MercatorBounds::FromLatLon(lat, lon));

    return (name.empty() ? 0 : jni::ToJavaString(env, name));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetViewportCountryNameIfAbsent(JNIEnv * env, jobject thiz)
  {
    string const name = g_framework->GetCountryNameIfAbsent(g_framework->GetViewportCenter());
    return (name.empty() ? 0 : jni::ToJavaString(env, name));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_Framework_nativeGetCountryIndex(JNIEnv * env, jobject thiz,
      jdouble lat, jdouble lon)
  {
    TIndex const idx = g_framework->GetCountryIndex(lat, lon);

    // Return 0 if no any country.
    if (idx.IsValid())
      return ToJava(idx);
    else
      return 0;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeShowCountry(JNIEnv * env, jobject thiz, jobject idx, jboolean zoomToDownloadButton)
  {
    g_framework->ShowCountry(ToNative(idx), (bool) zoomToDownloadButton);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_nativeSetRoutingListener(JNIEnv * env, jobject thiz, jobject listener)
  {
    frm()->SetRouteBuildingListener(bind(&CallRoutingListener, jni::make_global_ref(listener), _1, _2));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_downloadCountry(JNIEnv * env, jobject thiz, jobject idx)
  {
    storage_utils::GetMapLayout().DownloadMap(storage::ToNative(idx), TMapOptions::EMapOnly);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_deactivatePopup(JNIEnv * env, jobject thiz)
  {
    return g_framework->DeactivatePopup();
  }

  JNIEXPORT jdoubleArray JNICALL
  Java_com_mapswithme_maps_Framework_predictLocation(JNIEnv * env, jobject thiz, jdouble lat, jdouble lon, jdouble accuracy,
                                                     jdouble bearing, jdouble speed, jdouble elapsedSeconds)
  {
    double latitude = lat;
    double longitude = lon;
    ::Framework::PredictLocation(lat, lon, accuracy, bearing, speed, elapsedSeconds);
    double latlon[] = { lat, lon };
    jdoubleArray jLatLon = env->NewDoubleArray(2);
    env->SetDoubleArrayRegion(jLatLon, 0, 2, latlon);

    return jLatLon;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_Framework_setMapStyle(JNIEnv * env, jclass thiz, jint mapStyle)
  {
    MapStyle const val = static_cast<MapStyle>(mapStyle);
    android::Platform::RunOnGuiThreadImpl(bind(&android::Framework::SetMapStyle, g_framework, val));
  }
} // extern "C"
