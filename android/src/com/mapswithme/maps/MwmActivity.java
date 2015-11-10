package com.mapswithme.maps;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AlertDialog;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.Toast;
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.DownloadActivity;
import com.mapswithme.country.DownloadFragment;
import com.mapswithme.maps.Framework.OnBalloonListener;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.ads.LikesManager;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.ApiPoint;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPredictor;
import com.mapswithme.maps.routing.*;
import com.mapswithme.maps.search.FloatingSearchToolbarController;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.FadeView;
import com.mapswithme.maps.widget.menu.MainMenu;
import com.mapswithme.maps.widget.placepage.BasePlacePageAnimationController;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.*;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.io.Serializable;
import java.util.Stack;


public class MwmActivity extends BaseMwmFragmentActivity
                      implements LocationHelper.LocationListener,
                                 OnBalloonListener,
                                 View.OnTouchListener,
                                 BasePlacePageAnimationController.OnVisibilityChangedListener,
                                 OnClickListener,
                                 MapFragment.MapRenderingListener,
                                 CustomNavigateUpListener,
                                 ChooseBookmarkCategoryFragment.Listener,
                                 RoutingController.Container
{
  public static final String EXTRA_TASK = "map_task";
  private final static String EXTRA_CONSUMED = "mwm.extra.intent.processed";
  private static final String EXTRA_UPDATE_COUNTRIES = ".extra.update.countries";

  private static final String[] DOCKED_FRAGMENTS = { SearchFragment.class.getName(),
                                                     DownloadFragment.class.getName(),
                                                     RoutingPlanFragment.class.getName() };
  // Instance state
  private static final String STATE_PP_OPENED = "PpOpened";
  private static final String STATE_MAP_OBJECT = "MapObject";

  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();
  private final StoragePathManager mPathManager = new StoragePathManager();

  private View mFrame;

  // map
  private MapFragment mMapFragment;
  // Place page
  private PlacePageView mPlacePage;
  // Routing
  private RoutingPlanInplace mRoutingPlanInplace;
  private NavigationController mNavigationController;

  private MainMenu mMainMenu;
  private PanelAnimator mPanelAnimator;

  private int mLocationStateModeListenerId = LocationState.SLOT_UNDEFINED;

  private FadeView mFadeView;

  private ImageButton mBtnZoomIn;
  private ImageButton mBtnZoomOut;

  private boolean mIsFragmentContainer;

  private LocationPredictor mLocationPredictor;
  private FloatingSearchToolbarController mSearchController;
  private LastCompassData mLastCompassData;

  public interface LeftAnimationTrackListener
  {
    void onTrackStarted(boolean collapsed);

    void onTrackFinished(boolean collapsed);

    void onTrackLeftAnimation(float offset);
  }

  private static class LastCompassData
  {
    double magneticNorth;
    double trueNorth;
    double north;

    void update(int rotation, double magneticNorth, double trueNorth)
    {
      this.magneticNorth = LocationUtils.correctCompassAngle(rotation, magneticNorth);
      this.trueNorth = LocationUtils.correctCompassAngle(rotation, trueNorth);
      north = (this.trueNorth >= 0.0) ? this.trueNorth : this.magneticNorth;
    }
  }

  public static Intent createShowMapIntent(Context context, Index index, boolean doAutoDownload)
  {
    return new Intent(context, DownloadResourcesActivity.class)
        .putExtra(DownloadResourcesActivity.EXTRA_COUNTRY_INDEX, index)
        .putExtra(DownloadResourcesActivity.EXTRA_AUTODOWNLOAD_COUNTRY, doAutoDownload);
  }

  public static Intent createUpdateMapsIntent()
  {
    return new Intent(MwmApplication.get(), MwmActivity.class)
        .putExtra(EXTRA_UPDATE_COUNTRIES, true);
  }

  @Override
  public void onRenderingInitialized()
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        checkMeasurementSystem();
        checkKitkatMigrationMove();
      }
    });

    runTasks();
  }

  private void runTasks()
  {
    // Task are not UI-thread bounded,
    // if any task need UI-thread it should implicitly
    // use Activity.runOnUiThread().
    while (!mTasks.isEmpty())
      mTasks.pop().run(this);
  }

  private static void checkMeasurementSystem()
  {
    UnitLocale.initializeCurrentUnits();
  }

  private void checkKitkatMigrationMove()
  {
    mPathManager.checkKitkatMigration(this);
  }

  @Override
  protected int getFragmentContentResId()
  {
    return (mIsFragmentContainer ? R.id.fragment_container
                                 : super.getFragmentContentResId());
  }

  public Fragment getFragment(Class<? extends Fragment> clazz)
  {
    return (mIsFragmentContainer ? getSupportFragmentManager().findFragmentByTag(clazz.getName()) : null);
  }

  void replaceFragmentInternal(Class<? extends Fragment> fragmentClass, Bundle args)
  {
    super.replaceFragment(fragmentClass, args);
  }

  @Override
  public void replaceFragment(Class<? extends Fragment> fragmentClass, Bundle args)
  {
    replaceFragment(fragmentClass, args, null);
  }

  private void replaceFragment(Class<? extends Fragment> fragmentClass, Bundle args, Runnable completionListener)
  {
    mPanelAnimator.show(fragmentClass, args, completionListener);
  }

  private void showBookmarks()
  {
    startActivity(new Intent(this, BookmarkCategoriesActivity.class));
  }

  public void showSearch(String query)
  {
    showSearch(query, false);
  }

  public void showSearch(String query, boolean showMyPosition)
  {
    if (mIsFragmentContainer)
    {
      if (getFragment(SearchFragment.class) == null)
        popFragment();

      mSearchController.hide();

      final Bundle args = new Bundle();
      args.putString(SearchActivity.EXTRA_QUERY, query);
      args.putBoolean(SearchActivity.EXTRA_SHOW_MY_POSITION, showMyPosition);
      mPanelAnimator.show(SearchFragment.class, args);
    }
    else
      SearchActivity.start(this, query, showMyPosition);
  }

  private void shareMyLocation()
  {
    final Location loc = LocationHelper.INSTANCE.getLastLocation();
    if (loc != null)
    {
      final String geoUrl = Framework.nativeGetGe0Url(loc.getLatitude(), loc.getLongitude(), Framework.getDrawScale(), "");
      final String httpUrl = Framework.getHttpGe0Url(loc.getLatitude(), loc.getLongitude(), Framework.getDrawScale(), "");
      final String body = getString(R.string.my_position_share_sms, geoUrl, httpUrl);
      ShareOption.ANY.share(this, body);
      return;
    }

    new AlertDialog.Builder(MwmActivity.this)
        .setMessage(R.string.unknown_current_position)
        .setCancelable(true)
        .setPositiveButton(android.R.string.ok, new Dialog.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            dialog.dismiss();
          }
        }).show();
  }

  @Override
  public void showDownloader(boolean openDownloadedList)
  {
    final Bundle args = new Bundle();
    args.putBoolean(DownloadActivity.EXTRA_OPEN_DOWNLOADED_LIST, openDownloadedList);
    if (mIsFragmentContainer)
    {
      if (getFragment(DownloadFragment.class) != null) // downloader is already shown
        return;

      popFragment();
      SearchEngine.cancelSearch();
      mSearchController.refreshToolbar();
      mPanelAnimator.show(DownloadFragment.class, args);
    }
    else
    {
      startActivity(new Intent(this, DownloadActivity.class).putExtras(args));
    }
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_map);
    initViews();

    Statistics.INSTANCE.trackConnectionState();

    if (MwmApplication.get().nativeIsBenchmarking())
      Utils.keepScreenOn(true, getWindow());

    Framework.nativeSetBalloonListener(this);

    mSearchController = new FloatingSearchToolbarController(this);
    mLocationPredictor = new LocationPredictor(new Handler(), this);
    processIntent(getIntent());
    SharingHelper.prepare();
    RoutingController.get().attach(this);
  }

  private void initViews()
  {
    initMap();
    initYota();
    initPlacePage();
    initNavigationButtons();

    if (findViewById(R.id.fragment_container) != null)
      mIsFragmentContainer = true;
    else
      mRoutingPlanInplace = new RoutingPlanInplace(this);

    mNavigationController = new NavigationController(this);
    initMenu();
  }

  private void initMap()
  {
    mFrame = findViewById(R.id.map_fragment_container);

    mFadeView = (FadeView) findViewById(R.id.fade_view);
    mFadeView.setListener(new FadeView.Listener()
    {
      @Override
      public void onTouch()
      {
        mMainMenu.close(true);
      }
    });

    mMapFragment = (MapFragment) getSupportFragmentManager().findFragmentByTag(MapFragment.FRAGMENT_TAG);
    if (mMapFragment == null)
    {
      mMapFragment = (MapFragment) MapFragment.instantiate(this, MapFragment.class.getName(), null);
      getSupportFragmentManager()
          .beginTransaction()
          .replace(R.id.map_fragment_container, mMapFragment, MapFragment.FRAGMENT_TAG)
          .commit();
    }
    mFrame.setOnTouchListener(this);
  }

  private void initNavigationButtons()
  {
    View frame = findViewById(R.id.navigation_buttons);
    mBtnZoomIn = (ImageButton) frame.findViewById(R.id.map_button_plus);
    mBtnZoomIn.setOnClickListener(this);
    mBtnZoomOut = (ImageButton) frame.findViewById(R.id.map_button_minus);
    mBtnZoomOut.setOnClickListener(this);
  }

  private void initPlacePage()
  {
    mPlacePage = (PlacePageView) findViewById(R.id.info_box);
    mPlacePage.setOnVisibilityChangedListener(this);
    mPlacePage.findViewById(R.id.ll__route).setOnClickListener(this);
  }

  private void initYota()
  {
    if (Yota.isFirstYota())
      findViewById(R.id.yop_it).setOnClickListener(this);
  }

  private boolean closePlacePage()
  {
    if (mPlacePage.getState() == State.HIDDEN)
      return false;

    mPlacePage.hide();
    Framework.deactivatePopup();
    return true;
  }

  private boolean closeSidePanel()
  {
    if (canFragmentInterceptBackPress())
      return true;

    if (popFragment())
    {
      InputUtils.hideKeyboard(mFadeView);
      mFadeView.fadeOut(false);
      return true;
    }

    return false;
  }

  private void closeMenuAndRun(String statEvent, Runnable proc)
  {
    AlohaHelper.logClick(statEvent);

    mFadeView.fadeOut(false);
    mMainMenu.close(true, proc);
  }

  private void startLocationToPoint(String statEvent, final @Nullable MapObject endPoint)
  {
    closeMenuAndRun(statEvent, new Runnable()
    {
      @Override
      public void run()
      {
        RoutingController.get().prepare(endPoint);

        if (mPlacePage.isDocked() || !mPlacePage.isFloating())
          closePlacePage();
      }
    });
  }

  private void setRoutingStartButton(View button)
  {
    RoutingController.get().setStartButton(button);
    button.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        closeMenuAndRun(AlohaHelper.ROUTING_GO, new Runnable()
        {
          @Override
          public void run()
          {
            RoutingController.get().start();
          }
        });
      }
    });
  }

  private void toggleMenu()
  {
    if (mMainMenu.isOpen())
      mFadeView.fadeOut(false);
    else
      mFadeView.fadeIn();

    mMainMenu.toggle(true);
  }

  private void initMenu()
  {
    mMainMenu = new MainMenu((ViewGroup) findViewById(R.id.menu_frame), new MainMenu.Container()
    {
      @Override
      public Activity getActivity()
      {
        return MwmActivity.this;
      }

      @Override
      public void onItemClick(MainMenu.Item item)
      {
        switch (item)
        {
        case TOGGLE:
          if (!mMainMenu.isOpen())
          {
            if (mPlacePage.isDocked() && closePlacePage())
              return;

            if (closeSidePanel())
              return;
          }

          AlohaHelper.logClick(AlohaHelper.TOOLBAR_MENU);
          toggleMenu();
          break;

        case SEARCH:
          closeMenuAndRun(AlohaHelper.TOOLBAR_SEARCH, new Runnable()
          {
            @Override
            public void run()
            {
              showSearch(mSearchController.getQuery());
            }
          });
          break;

        case P2P:
          startLocationToPoint(AlohaHelper.MENU_POINT2POINT, null);
          break;

        case BOOKMARKS:
          closeMenuAndRun(AlohaHelper.TOOLBAR_BOOKMARKS, new Runnable()
          {
            @Override
            public void run()
            {
              showBookmarks();
            }
          });
          break;

        case SHARE:
          closeMenuAndRun(AlohaHelper.MENU_SHARE, new Runnable()
          {
            @Override
            public void run()
            {
              shareMyLocation();
            }
          });
          break;

        case DOWNLOADER:
          closeMenuAndRun(AlohaHelper.MENU_DOWNLOADER, new Runnable()
          {
            @Override
            public void run()
            {
              showDownloader(false);
            }
          });
          break;

        case SETTINGS:
          closeMenuAndRun(AlohaHelper.MENU_SETTINGS, new Runnable()
          {
            @Override
            public void run()
            {
              startActivity(new Intent(getActivity(), SettingsActivity.class));
            }
          });
          break;
        }
      }
    });

    if (mPlacePage.isDocked())
    {
      mPlacePage.setLeftAnimationTrackListener(mMainMenu.getLeftAnimationTrackListener());
      return;
    }

    if (mIsFragmentContainer)
      mPanelAnimator = new PanelAnimator(this, mMainMenu.getLeftAnimationTrackListener());

    View start = mMainMenu.getRouteStartButton();
    if (start != null)
      setRoutingStartButton(start);
  }

  @Override
  public void onDestroy()
  {
    Framework.nativeRemoveBalloonListener();
    BottomSheetHelper.free();
    RoutingController.get().detach();
    super.onDestroy();
  }

  @Override
  protected void onSaveInstanceState(Bundle outState)
  {
    if (mPlacePage.getState() != State.HIDDEN)
    {
      outState.putBoolean(STATE_PP_OPENED, true);
      outState.putParcelable(STATE_MAP_OBJECT, mPlacePage.getMapObject());
    }

    mMainMenu.onSaveState(outState);
    super.onSaveInstanceState(outState);
  }

  @Override
  protected void onRestoreInstanceState(@NonNull Bundle savedInstanceState)
  {
    super.onRestoreInstanceState(savedInstanceState);

    if (savedInstanceState.getBoolean(STATE_PP_OPENED))
    {
      mPlacePage.setMapObject((MapObject) savedInstanceState.getParcelable(STATE_MAP_OBJECT));
      mPlacePage.setState(State.PREVIEW);
    }

//    mMainMenu.onRestoreState(savedInstanceState, mLayoutRouting.getState());
//    if (mMainMenu.shouldOpenDelayed())
//      mFadeView.fadeInInstantly();
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    processIntent(intent);
  }

  private void processIntent(Intent intent)
  {
    if (intent == null)
      return;

    if (intent.hasExtra(EXTRA_TASK))
      addTask(intent);
    else if (intent.hasExtra(EXTRA_UPDATE_COUNTRIES))
    {
      ActiveCountryTree.updateAll();
      showDownloader(true);
    }
  }

  private void addTask(Intent intent)
  {
    if (intent != null &&
        !intent.getBooleanExtra(EXTRA_CONSUMED, false) &&
        intent.hasExtra(EXTRA_TASK) &&
        ((intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) == 0))
    {
      final MapTask mapTask = (MapTask) intent.getSerializableExtra(EXTRA_TASK);
      mTasks.add(mapTask);
      intent.removeExtra(EXTRA_TASK);

      if (mMapFragment.isRenderingInitialized())
        runTasks();

      // mark intent as consumed
      intent.putExtra(EXTRA_CONSUMED, true);
    }
  }

  @Override
  public void onLocationError(int errorCode)
  {
    mMapFragment.nativeOnLocationError(errorCode);

    if (errorCode == LocationHelper.ERROR_DENIED)
    {
      LocationState.INSTANCE.turnOff();

      Intent intent = new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS);
      if (intent.resolveActivity(getPackageManager()) == null)
      {
        intent = new Intent(android.provider.Settings.ACTION_SECURITY_SETTINGS);
        if (intent.resolveActivity(getPackageManager()) == null)
          return;
      }

      final Intent finIntent = intent;
      new AlertDialog.Builder(this)
          .setTitle(R.string.enable_location_service)
          .setMessage(R.string.location_is_disabled_long_text)
          .setPositiveButton(R.string.connection_settings, new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
              startActivity(finIntent);
            }
          })
          .setNegativeButton(R.string.close, null)
          .show();
    }
    else if (errorCode == LocationHelper.ERROR_GPS_OFF)
    {
      Toast.makeText(this, R.string.gps_is_disabled_long_text, Toast.LENGTH_LONG).show();
    }
  }

  @Override
  public void onLocationUpdated(final Location location)
  {
    if (!location.getProvider().equals(LocationHelper.LOCATION_PREDICTOR_PROVIDER))
      mLocationPredictor.reset(location);

    mMapFragment.nativeLocationUpdated(location.getTime(),
                                       location.getLatitude(),
                                       location.getLongitude(),
                                       location.getAccuracy(),
                                       location.getAltitude(),
                                       location.getSpeed(),
                                       location.getBearing());

    if (mPlacePage.getState() != State.HIDDEN)
      mPlacePage.refreshLocation(location);

    if (!RoutingController.get().isNavigating())
      return;

    RoutingInfo info = Framework.nativeGetRouteFollowingInfo();
    mNavigationController.update(info);
    mMainMenu.updateRoutingInfo(info);

    TtsPlayer.INSTANCE.playTurnNotifications();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    if (mLastCompassData == null)
      mLastCompassData = new LastCompassData();

    mLastCompassData.update(getWindowManager().getDefaultDisplay().getRotation(), magneticNorth, trueNorth);
    mMapFragment.nativeCompassUpdated(mLastCompassData.magneticNorth, mLastCompassData.trueNorth, false);

    mPlacePage.refreshAzimuth(mLastCompassData.north);
    mNavigationController.updateNorth(mLastCompassData.north);
  }

  // Callback from native location state mode element processing.
  @SuppressWarnings("unused")
  public void onLocationStateModeChangedCallback(final int newMode)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        refreshLocationState(newMode);
      }
    });
  }

  private void refreshLocationState(int newMode)
  {
    mMainMenu.getMyPositionButton().update(newMode);

    switch (newMode)
    {
    case LocationState.UNKNOWN_POSITION:
      pauseLocation();
      break;
    case LocationState.PENDING_POSITION:
      resumeLocation();
      break;
    }
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    mLocationStateModeListenerId = LocationState.INSTANCE.addLocationStateModeListener(this);
    invalidateLocationState();

    mSearchController.refreshToolbar();
    mPlacePage.onResume();
    LikesManager.INSTANCE.showDialogs(this);
    mMainMenu.onResume();
  }

  @Override
  protected void onStart()
  {
    super.onStart();

    if (!mIsFragmentContainer)
      popFragment();
  }

  private void adjustZoomButtons()
  {
    boolean show = (RoutingController.get().isNavigating() || Config.showZoomButtons());
    UiUtils.showIf(show, mBtnZoomIn, mBtnZoomOut);

    if (!show)
      return;

    mFrame.post(new Runnable()
    {
      @Override
      public void run()
      {
        int height = mFrame.getMeasuredHeight();
        int top = UiUtils.dimen(R.dimen.zoom_buttons_top_required_space);
        int bottom = UiUtils.dimen(R.dimen.zoom_buttons_bottom_max_space);

        int space = (top + bottom < height ? bottom : height - top);

        LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) mBtnZoomOut.getLayoutParams();
        lp.bottomMargin = space;
        mBtnZoomOut.setLayoutParams(lp);
      }
    });
  }

  @Override
  protected void onPause()
  {
    LocationState.INSTANCE.removeLocationStateModeListener(mLocationStateModeListenerId);
    pauseLocation();
    TtsPlayer.INSTANCE.stop();
    LikesManager.INSTANCE.cancelDialogs();
    super.onPause();
  }

  private void resumeLocation()
  {
    LocationHelper.INSTANCE.addLocationListener(this);
    // Do not turn off the screen while displaying position
    Utils.keepScreenOn(true, getWindow());
    mLocationPredictor.resume();
  }

  private void pauseLocation()
  {
    LocationHelper.INSTANCE.removeLocationListener(this);
    // Enable automatic turning screen off while app is idle
    Utils.keepScreenOn(false, getWindow());
    mLocationPredictor.pause();
  }

  /**
   * Invalidates location state in core.
   * Updates location button accordingly.
   */
  public void invalidateLocationState()
  {
    final int currentLocationMode = LocationState.INSTANCE.getLocationStateMode();
    refreshLocationState(currentLocationMode);
    LocationState.INSTANCE.invalidatePosition();
  }

  @Override
  public void onBackPressed()
  {
    if (mMainMenu.close(true))
    {
      mFadeView.fadeOut(false);
      return;
    }

    if (mSearchController.hide())
    {
      SearchEngine.cancelSearch();
      return;
    }

    if (!closePlacePage() && !closeSidePanel() && !RoutingController.get().cancel())
      super.onBackPressed();
  }

  private boolean isMapFaded()
  {
    return mFadeView.getVisibility() == View.VISIBLE;
  }

  private boolean canFragmentInterceptBackPress()
  {
    final FragmentManager manager = getSupportFragmentManager();
    for (String tag : DOCKED_FRAGMENTS)
    {
      final Fragment fragment = manager.findFragmentByTag(tag);
      if (fragment != null && fragment.isResumed() && fragment instanceof OnBackPressListener)
        return ((OnBackPressListener) fragment).onBackPressed();
    }

    return false;
  }

  private boolean popFragment()
  {
    for (String tag : DOCKED_FRAGMENTS)
      if (popFragment(tag))
        return true;

    return false;
  }

  private boolean popFragment(String className)
  {
    final FragmentManager manager = getSupportFragmentManager();
    Fragment fragment = manager.findFragmentByTag(className);

    // TODO d.yunitsky
    // we cant pop fragment, if it isn't resumed, cause of 'at android.support.v4.app.FragmentManagerImpl.checkStateLoss(FragmentManager.java:1375)'
    if (fragment == null || !fragment.isResumed())
      return false;

    if (mPanelAnimator == null)
      return false;

    mPanelAnimator.hide(new Runnable()
    {
      @Override
      public void run()
      {
        manager.popBackStackImmediate();
      }
    });

    return true;
  }

  // Callbacks from native map objects touch event.
  @Override
  public void onApiPointActivated(final double lat, final double lon, final String name, final String id)
  {
    final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
    if (request == null)
      return;

    request.setPointData(lat, lon, name, id);

    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        final String poiType = request.getCallerName(MwmApplication.get()).toString();
        activateMapObject(new ApiPoint(name, id, poiType, lat, lon));
      }
    });
  }

  @Override
  public void onPoiActivated(final String name, final String type, final String address, final double lat, final double lon,
                             final int[] metaTypes, final String[] metaValues)
  {
    final MapObject poi = new MapObject.Poi(name, lat, lon, type);
    poi.addMetadata(metaTypes, metaValues);

    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        activateMapObject(poi);
      }
    });
  }

  @Override
  public void onBookmarkActivated(final int category, final int bookmarkIndex)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        activateMapObject(BookmarkManager.INSTANCE.getBookmark(category, bookmarkIndex));
      }
    });
  }

  @Override
  public void onMyPositionActivated(final double lat, final double lon)
  {
    final MapObject mypos = new MapObject.MyPosition(lat, lon);

    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        if (!Framework.nativeIsRoutingActive())
        {
          activateMapObject(mypos);
        }
      }
    });
  }

  @Override
  public void onAdditionalLayerActivated(final String name, final String type, final double lat, final double lon, final int[] metaTypes, final String[] metaValues)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        final MapObject sr = new MapObject.SearchResult(name, type, lat, lon);
        sr.addMetadata(metaTypes, metaValues);
        activateMapObject(sr);
      }
    });
  }

  private void activateMapObject(MapObject object)
  {
    if (!mPlacePage.hasMapObject(object))
    {
      mPlacePage.setMapObject(object);
      mPlacePage.setState(State.PREVIEW);

      if (isMapFaded())
        mFadeView.fadeOut(false);
    }
  }

  @Override
  public void onDismiss()
  {
    if (!mPlacePage.hasMapObject(null))
    {
      runOnUiThread(new Runnable()
      {
        @Override
        public void run()
        {
          mPlacePage.hide();
          Framework.deactivatePopup();
        }
      });
    }
  }

  @Override
  public void onPreviewVisibilityChanged(boolean isVisible)
  {
    if (!isVisible)
    {
      Framework.deactivatePopup();
      mPlacePage.setMapObject(null);
      mMainMenu.show(true);
    }
  }

  @Override
  public void onPlacePageVisibilityChanged(boolean isVisible)
  {
    AlohaHelper.logClick(isVisible ? AlohaHelper.PP_OPEN
                                   : AlohaHelper.PP_CLOSE);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.ll__route:
      startLocationToPoint(AlohaHelper.PP_ROUTE, mPlacePage.getMapObject());
      break;

    case R.id.map_button_plus:
      AlohaHelper.logClick(AlohaHelper.ZOOM_IN);
      mMapFragment.nativeScale(3.0 / 2);
      break;
    case R.id.map_button_minus:
      AlohaHelper.logClick(AlohaHelper.ZOOM_OUT);
      mMapFragment.nativeScale(2 / 3.0);
      break;
    case R.id.yop_it:
      final double[] latLon = Framework.getScreenRectCenter();
      final double zoom = Framework.getDrawScale();

      final int locationStateMode = LocationState.INSTANCE.getLocationStateMode();

      if (locationStateMode > LocationState.NOT_FOLLOW)
        Yota.showLocation(getApplicationContext(), zoom);
      else
        Yota.showMap(getApplicationContext(), latLon[0], latLon[1], zoom, null, locationStateMode == LocationState.NOT_FOLLOW);

      Statistics.INSTANCE.trackBackscreenCall("Map");
      break;
    }
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    return mPlacePage.hideOnTouch() ||
           mMapFragment.onTouch(view, event);
  }

  @Override
  public void customOnNavigateUp()
  {
    if (popFragment())
    {
      InputUtils.hideKeyboard(mMainMenu.getFrame());
      mSearchController.refreshToolbar();
    }
  }

  public interface MapTask extends Serializable
  {
    boolean run(MwmActivity target);
  }

  public static class OpenUrlTask implements MapTask
  {
    private static final long serialVersionUID = 1L;
    private final String mUrl;

    public OpenUrlTask(String url)
    {
      Utils.checkNotNull(url);
      mUrl = url;
    }

    @Override
    public boolean run(MwmActivity target)
    {
      return target.mMapFragment.showMapForUrl(mUrl);
    }
  }

  public static class ShowCountryTask implements MapTask
  {
    private static final long serialVersionUID = 1L;
    private final Index mIndex;
    private final boolean mDoAutoDownload;

    public ShowCountryTask(Index index, boolean doAutoDownload)
    {
      mIndex = index;
      mDoAutoDownload = doAutoDownload;
    }

    @Override
    public boolean run(MwmActivity target)
    {
      if (mDoAutoDownload)
      {
        Framework.downloadCountry(mIndex);
        // set zoom level so that download process is visible
        Framework.nativeShowCountry(mIndex, true);
      }
      else
        Framework.nativeShowCountry(mIndex, false);

      return true;
    }
  }

  public void adjustCompass(int offset)
  {
    if (mMapFragment != null && mMapFragment.isAdded())
    {
      mMapFragment.adjustCompass(mPanelAnimator.isVisible() ? offset : 0);

      if (mLastCompassData != null)
        mMapFragment.nativeCompassUpdated(mLastCompassData.magneticNorth, mLastCompassData.trueNorth, true);
    }
  }

  @Override
  public void onCategoryChanged(int bookmarkId, int newCategoryId)
  {
    mPlacePage.setMapObject(BookmarkManager.INSTANCE.getBookmark(newCategoryId, bookmarkId));
  }

  @Override
  public FragmentActivity getActivity()
  {
    return this;
  }

  @Override
  public void showSearch(boolean showMyPosition)
  {
    showSearch("", showMyPosition);
  }

  @Override
  public void updateMenu()
  {
    if (RoutingController.get().isNavigating())
    {
      mMainMenu.setState(MainMenu.State.NAVIGATION);
      return;
    }

    if (mIsFragmentContainer)
    {
      // TODO: Handle point selection state
      mMainMenu.setEnabled(MainMenu.Item.P2P, !RoutingController.get().isPlanning());
    } else
    {
      if (RoutingController.get().isPlanning())
      {
        mMainMenu.setState(MainMenu.State.ROUTE_PREPARE);
        return;
      }
    }

    mMainMenu.setState(MainMenu.State.MENU);
  }

  @Override
  public void showRoutePlan(boolean show)
  {
    if (show)
    {
      mSearchController.hide();

      if (mIsFragmentContainer)
      {
        Fragment f = getFragment(RoutingPlanFragment.class);
        if (f == null)
          popFragment();

        mPanelAnimator.show(RoutingPlanFragment.class, null);
      } else
      {
        mRoutingPlanInplace.show(true);
      }
    } else
    {
      if (mIsFragmentContainer)
        closeSidePanel();
      else
        mRoutingPlanInplace.show(false);
    }

    mPlacePage.refreshViews();
  }

  @Override
  public void showNavigation(boolean show)
  {
    adjustZoomButtons();
    mMainMenu.setState(show ? MainMenu.State.NAVIGATION
                            : MainMenu.State.MENU);
    mPlacePage.refreshViews();
    mNavigationController.show(show);
  }

  @Override
  public void updatePoints()
  {
    if (mIsFragmentContainer)
    {
      // TODO
    } else
    {
      mRoutingPlanInplace.updatePoints();
    }
  }

  @Override
  public void updateBuildProgress(int progress, int router)
  {
    if (mIsFragmentContainer)
    {
      // TODO
    } else
    {
      mRoutingPlanInplace.updateBuildProgress(progress, router);
    }
  }
}
