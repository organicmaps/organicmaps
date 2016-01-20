package com.mapswithme.maps;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.location.Location;
import android.os.Build;
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
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.Toast;

import java.io.Serializable;
import java.util.Stack;

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
import com.mapswithme.maps.routing.NavigationController;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.routing.RoutingInfo;
import com.mapswithme.maps.routing.RoutingPlanFragment;
import com.mapswithme.maps.routing.RoutingPlanInplaceController;
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
import com.mapswithme.util.Animations;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.MytargetHelper;
import com.mapswithme.util.statistics.Statistics;
import ru.mail.android.mytarget.nativeads.NativeAppwallAd;
import ru.mail.android.mytarget.nativeads.banners.NativeAppwallBanner;

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
  private static final String EXTRA_CONSUMED = "mwm.extra.intent.processed";
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
  private RoutingPlanInplaceController mRoutingPlanInplaceController;
  private NavigationController mNavigationController;

  private MainMenu mMainMenu;
  private PanelAnimator mPanelAnimator;
  private MytargetHelper mMytargetHelper;

  private FadeView mFadeView;

  private ImageButton mBtnZoomIn;
  private ImageButton mBtnZoomOut;

  private boolean mIsFragmentContainer;
  private boolean mIsFullscreen;

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
    checkMeasurementSystem();
    checkKitkatMigrationMove();

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

  public @Nullable Fragment getFragment(Class<? extends Fragment> clazz)
  {
    if (!mIsFragmentContainer)
      throw new IllegalStateException("Must be called for tablets only!");

    return getSupportFragmentManager().findFragmentByTag(clazz.getName());
  }

  void replaceFragmentInternal(Class<? extends Fragment> fragmentClass, Bundle args)
  {
    super.replaceFragment(fragmentClass, args, null);
  }

  @Override
  public void replaceFragment(Class<? extends Fragment> fragmentClass, Bundle args, Runnable completionListener)
  {
    if (mPanelAnimator.isVisible() && getFragment(fragmentClass) != null)
    {
      if (completionListener != null)
        completionListener.run();
      return;
    }

    mPanelAnimator.show(fragmentClass, args, completionListener);
  }

  private void showBookmarks()
  {
    startActivity(new Intent(this, BookmarkCategoriesActivity.class));
  }

  public void showSearch(String query)
  {
    if (mIsFragmentContainer)
    {
      mSearchController.hide();

      final Bundle args = new Bundle();
      args.putString(SearchActivity.EXTRA_QUERY, query);
      replaceFragment(SearchFragment.class, args, null);
    }
    else
      SearchActivity.start(this, query);
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
      SearchEngine.cancelSearch();
      mSearchController.refreshToolbar();
      replaceFragment(DownloadFragment.class, args, null);
    }
    else
    {
      startActivity(new Intent(this, DownloadActivity.class).putExtras(args));
    }
  }

  @Override
  public int getThemeResourceId(String theme)
  {
    if (ThemeUtils.isDefaultTheme(theme))
      return R.style.MwmTheme_MainActivity;

    if (ThemeUtils.isNightTheme(theme))
      return R.style.MwmTheme_Night_MainActivity;

    return super.getThemeResourceId(theme);
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    mIsFragmentContainer = getResources().getBoolean(R.bool.tabletLayout);

    if (!mIsFragmentContainer && (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP))
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

    setContentView(R.layout.activity_map);
    initViews();

    Statistics.INSTANCE.trackConnectionState();

    Framework.nativeSetBalloonListener(this);

    mSearchController = new FloatingSearchToolbarController(this);
    mLocationPredictor = new LocationPredictor(new Handler(), this);
    processIntent(getIntent());
    SharingHelper.prepare();
  }

  private void initViews()
  {
    initMap();
    initYota();
    initPlacePage();
    initNavigationButtons();

    if (!mIsFragmentContainer)
    {
      mRoutingPlanInplaceController = new RoutingPlanInplaceController(this);
      removeCurrentFragment(false);
    }

    mNavigationController = new NavigationController(this);
    RoutingController.get().attach(this);
    initMenu();
  }

  private void initMap()
  {
    mFrame = findViewById(R.id.map_fragment_container);

    mFadeView = (FadeView) findViewById(R.id.fade_view);
    mFadeView.setListener(new FadeView.Listener() {
      @Override
      public void onTouch() {
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
    mBtnZoomIn.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.zoom_in_night
                                                          : R.drawable.zoom_in);
    mBtnZoomIn.setOnClickListener(this);
    mBtnZoomOut = (ImageButton) frame.findViewById(R.id.map_button_minus);
    mBtnZoomOut.setOnClickListener(this);
    mBtnZoomOut.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.zoom_out_night
                                                           : R.drawable.zoom_out);
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
    if (interceptBackPress())
      return true;

    if (removeCurrentFragment(true))
    {
      InputUtils.hideKeyboard(mFadeView);
      mFadeView.fadeOut(false);
      return true;
    }

    return false;
  }

  public void closeMenu(String statEvent, String alohaStatEvent, @Nullable Runnable procAfterClose)
  {
    Statistics.INSTANCE.trackEvent(statEvent);
    AlohaHelper.logClick(alohaStatEvent);

    mFadeView.fadeOut(false);
    mMainMenu.close(true, procAfterClose);
  }

  private void startLocationToPoint(String statisticsEvent, String alohaEvent, final @Nullable MapObject endPoint)
  {
    closeMenu(statisticsEvent, alohaEvent, new Runnable() {
      @Override
      public void run() {
        RoutingController.get().prepare(endPoint);

        if (mPlacePage.isDocked() || !mPlacePage.isFloating())
          closePlacePage();
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

          Statistics.INSTANCE.trackEvent(Statistics.EventName.TOOLBAR_MENU);
          AlohaHelper.logClick(AlohaHelper.TOOLBAR_MENU);
          toggleMenu();
          break;

        case SEARCH:
          RoutingController.get().cancelPlanning();
          closeMenu(Statistics.EventName.TOOLBAR_SEARCH, AlohaHelper.TOOLBAR_SEARCH, new Runnable()
          {
            @Override
            public void run()
            {
              showSearch(mSearchController.getQuery());
            }
          });
          break;

        case P2P:
          startLocationToPoint(Statistics.EventName.MENU_P2P, AlohaHelper.MENU_POINT2POINT, null);
          break;

        case BOOKMARKS:
          closeMenu(Statistics.EventName.TOOLBAR_BOOKMARKS, AlohaHelper.TOOLBAR_BOOKMARKS, new Runnable()
          {
            @Override
            public void run()
            {
              showBookmarks();
            }
          });
          break;

        case SHARE:
          closeMenu(Statistics.EventName.MENU_SHARE, AlohaHelper.MENU_SHARE, new Runnable()
          {
            @Override
            public void run()
            {
              shareMyLocation();
            }
          });
          break;

        case DOWNLOADER:
          RoutingController.get().cancelPlanning();
          closeMenu(Statistics.EventName.MENU_DOWNLOADER, AlohaHelper.MENU_DOWNLOADER, new Runnable()
          {
            @Override
            public void run()
            {
              showDownloader(false);
            }
          });
          break;

        case SETTINGS:
          closeMenu(Statistics.EventName.MENU_SETTINGS, AlohaHelper.MENU_SETTINGS, new Runnable()
          {
            @Override
            public void run()
            {
              startActivity(new Intent(getActivity(), SettingsActivity.class));
            }
          });
          break;

        case SHOWCASE:
          closeMenu(Statistics.EventName.MENU_SHOWCASE, AlohaHelper.MENU_SHOWCASE, new Runnable()
          {
            @Override
            public void run()
            {
              mMytargetHelper.displayShowcase();
            }
          });
          break;
        }
      }
    });

    if (mIsFragmentContainer)
    {
      mPanelAnimator = new PanelAnimator(this, mMainMenu.getLeftAnimationTrackListener());
      return;
    }

    mRoutingPlanInplaceController.setStartButton();
    if (mPlacePage.isDocked())
      mPlacePage.setLeftAnimationTrackListener(mMainMenu.getLeftAnimationTrackListener());
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
    if (!mIsFragmentContainer && RoutingController.get().isPlanning())
      mRoutingPlanInplaceController.onSaveState(outState);
    RoutingController.get().onSaveState();
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

    if (!mIsFragmentContainer && RoutingController.get().isPlanning())
      mRoutingPlanInplaceController.restoreState(savedInstanceState);
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

      if (MapFragment.nativeIsEngineCreated())
        runTasks();

      // mark intent as consumed
      intent.putExtra(EXTRA_CONSUMED, true);
    }
  }

  @Override
  public void onLocationError(int errorCode)
  {
    LocationHelper.nativeOnLocationError(errorCode);

    if (errorCode == LocationHelper.ERROR_DENIED)
    {
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

    LocationHelper.onLocationUpdated(location);

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
    MapFragment.nativeCompassUpdated(mLastCompassData.magneticNorth, mLastCompassData.trueNorth, false);

    mPlacePage.refreshAzimuth(mLastCompassData.north);
    mNavigationController.updateNorth(mLastCompassData.north);
  }

  // Callback from native location state mode element processing.
  @SuppressWarnings("unused")
  public void onMyPositionModeChangedCallback(final int newMode)
  {
    mLocationPredictor.myPositionModeChanged(newMode);
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

    LocationState.INSTANCE.setMyPositionModeListener(this);
    invalidateLocationState();

    mSearchController.refreshToolbar();
    mPlacePage.onResume();

    if (!RoutingController.get().isNavigating())
    {
      if (!NewsFragment.showOn(this))
        LikesManager.INSTANCE.showDialogs(this);
    }

    mMainMenu.onResume();
  }

  @Override
  public void recreate()
  {
    // Explicitly destroy engine before activity recreation.
    mMapFragment.destroyEngine();
    super.recreate();
  }

  private void initShowcase()
  {
    NativeAppwallAd.AppwallAdListener listener = new NativeAppwallAd.AppwallAdListener()
    {
      @Override
      public void onLoad(NativeAppwallAd nativeAppwallAd)
      {
        if (nativeAppwallAd.getBanners().isEmpty())
        {
          mMainMenu.showShowcase(false);
          return;
        }

        mMainMenu.showShowcase(true);
      }

      @Override
      public void onNoAd(String reason, NativeAppwallAd nativeAppwallAd)
      {
        mMainMenu.showShowcase(false);
      }

      @Override
      public void onClick(NativeAppwallBanner nativeAppwallBanner, NativeAppwallAd nativeAppwallAd) {}

      @Override
      public void onDismissDialog(NativeAppwallAd nativeAppwallAd) {}
    };
    mMytargetHelper = new MytargetHelper(listener, this);
  }

  @Override
  protected void onResumeFragments()
  {
    super.onResumeFragments();
    RoutingController.get().restore();
    mPlacePage.restore();
  }

  private void adjustZoomButtons()
  {
    final boolean show = showZoomButtons();
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

  private boolean showZoomButtons()
  {
    return RoutingController.get().isNavigating() || Config.showZoomButtons();
  }

  @Override
  protected void onPause()
  {
    LocationState.INSTANCE.removeMyPositionModeListener();
    pauseLocation();
    TtsPlayer.INSTANCE.stop();
    LikesManager.INSTANCE.cancelDialogs();
    super.onPause();
  }

  private void resumeLocation()
  {
    LocationHelper.INSTANCE.addLocationListener(this, true);
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
    default:
      break;
    }
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
  protected void onStart()
  {
    super.onStart();
    initShowcase();
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    mMytargetHelper.cancel();
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

  private boolean interceptBackPress()
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

  private void removeFragmentImmediate(Fragment fragment)
  {
    FragmentManager fm = getSupportFragmentManager();
    if (fm.isDestroyed())
      return;

    fm.beginTransaction()
      .remove(fragment)
      .commitAllowingStateLoss();
    fm.executePendingTransactions();
  }

  private boolean removeCurrentFragment(boolean animate)
  {
    for (String tag : DOCKED_FRAGMENTS)
      if (removeFragment(tag, animate))
        return true;

    return false;
  }

  private boolean removeFragment(String className, boolean animate)
  {
    if (animate && mPanelAnimator == null)
      animate = false;

    final Fragment fragment = getSupportFragmentManager().findFragmentByTag(className);
    if (fragment == null)
      return false;

    if (animate)
      mPanelAnimator.hide(new Runnable()
      {
        @Override
        public void run()
        {
          removeFragmentImmediate(fragment);
        }
      });
    else
      removeFragmentImmediate(fragment);

    return true;
  }

  // Callbacks from native touch events on map objects.
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
    activateMapObject(poi);
  }

  @Override
  public void onBookmarkActivated(final int category, final int bookmarkIndex)
  {
    activateMapObject(BookmarkManager.INSTANCE.getBookmark(category, bookmarkIndex));
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
    final MapObject sr = new MapObject.SearchResult(name, type, lat, lon);
    sr.addMetadata(metaTypes, metaValues);
    activateMapObject(sr);
  }

  private void activateMapObject(MapObject object)
  {
    setFullscreen(false);
    if (!mPlacePage.hasMapObject(object))
    {
      mPlacePage.setMapObject(object);
      mPlacePage.setState(State.PREVIEW);

      if (UiUtils.isVisible(mFadeView))
        mFadeView.fadeOut(false);
    }
  }

  @Override
  public void onDismiss()
  {
    if (!mPlacePage.hasMapObject(null))
      mPlacePage.hide();
    else
    {
      if ((mPanelAnimator != null && mPanelAnimator.isVisible()) ||
          UiUtils.isVisible(mSearchController.getToolbar()))
        return;

      setFullscreen(!mIsFullscreen);
    }
  }

  private void setFullscreen(boolean isFullscreen)
  {
    mIsFullscreen = isFullscreen;
    if (isFullscreen)
    {
      Animations.disappearSliding(mMainMenu.getFrame(), Animations.BOTTOM, new Runnable()
      {
        @Override
        public void run()
        {
          final int menuHeight = mMainMenu.getFrame().getHeight();
          adjustCompass(0, menuHeight);
          adjustRuler(0, menuHeight);
        }
      });
      if (showZoomButtons())
      {
        Animations.disappearSliding(mBtnZoomOut, Animations.RIGHT, null);
        Animations.disappearSliding(mBtnZoomIn, Animations.RIGHT, null);
      }
    }
    else
    {
      Animations.appearSliding(mMainMenu.getFrame(), Animations.BOTTOM, new Runnable()
      {
        @Override
        public void run()
        {
          adjustCompass(0, 0);
          adjustRuler(0, 0);
        }
      });
      if (showZoomButtons())
      {
        Animations.appearSliding(mBtnZoomOut, Animations.RIGHT, null);
        Animations.appearSliding(mBtnZoomIn, Animations.RIGHT, null);
      }
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
    Statistics.INSTANCE.trackEvent(isVisible ? Statistics.EventName.PP_OPEN
                                             : Statistics.EventName.PP_CLOSE);
    AlohaHelper.logClick(isVisible ? AlohaHelper.PP_OPEN
                                   : AlohaHelper.PP_CLOSE);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.ll__route:
      startLocationToPoint(Statistics.EventName.PP_ROUTE, AlohaHelper.PP_ROUTE, mPlacePage.getMapObject());
      break;
    case R.id.map_button_plus:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.ZOOM_IN);
      AlohaHelper.logClick(AlohaHelper.ZOOM_IN);
      MapFragment.nativeScalePlus();
      break;
    case R.id.map_button_minus:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.ZOOM_OUT);
      AlohaHelper.logClick(AlohaHelper.ZOOM_OUT);
      MapFragment.nativeScaleMinus();
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
    if (removeCurrentFragment(true))
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
      return MapFragment.nativeShowMapForUrl(mUrl);
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
        Framework.downloadCountry(mIndex);
      Framework.nativeShowCountry(mIndex, mDoAutoDownload);
      return true;
    }
  }

  public void adjustCompass(int offsetX, int offsetY)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    mMapFragment.setupCompass((mPanelAnimator != null && mPanelAnimator.isVisible()) ? offsetX : 0, offsetY, true);

    if (mLastCompassData != null)
      MapFragment.nativeCompassUpdated(mLastCompassData.magneticNorth, mLastCompassData.trueNorth, true);
  }

  public void adjustRuler(int offsetX, int offsetY)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    mMapFragment.setupRuler(offsetX, offsetY, true);
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

  public MainMenu getMainMenu()
  {
    return mMainMenu;
  }

  @Override
  public void showSearch()
  {
    showSearch("");
  }

  @Override
  public void updateMenu()
  {
    if (RoutingController.get().isNavigating())
    {
      mMainMenu.setState(MainMenu.State.NAVIGATION, false);
      return;
    }

    if (mIsFragmentContainer)
    {
      mMainMenu.setEnabled(MainMenu.Item.P2P, !RoutingController.get().isPlanning());
      mMainMenu.setEnabled(MainMenu.Item.SEARCH, !RoutingController.get().isWaitingPoiPick());
    }
    else if (RoutingController.get().isPlanning())
    {
      mMainMenu.setState(MainMenu.State.ROUTE_PREPARE, false);
      return;
    }

    mMainMenu.setState(MainMenu.State.MENU, false);
  }

  @Override
  public void showRoutePlan(boolean show, @Nullable Runnable completionListener)
  {
    if (show)
    {
      mSearchController.hide();

      if (mIsFragmentContainer)
      {
        replaceFragment(RoutingPlanFragment.class, null, completionListener);
      }
      else
      {
        mRoutingPlanInplaceController.show(true);
        if (completionListener != null)
          completionListener.run();
      }
    }
    else
    {
      if (mIsFragmentContainer)
        closeSidePanel();
      else
        mRoutingPlanInplaceController.show(false);

      if (completionListener != null)
        completionListener.run();
    }

    mPlacePage.refreshViews();
  }

  @Override
  public void showNavigation(boolean show)
  {
    adjustZoomButtons();
    mPlacePage.refreshViews();
    mNavigationController.show(show);
  }

  @Override
  public void updatePoints()
  {
    if (mIsFragmentContainer)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment)getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.updatePoints();
    }
    else
    {
      mRoutingPlanInplaceController.updatePoints();
    }
  }

  @Override
  public void updateBuildProgress(int progress, int router)
  {
    if (mIsFragmentContainer)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment)getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.updateBuildProgress(progress, router);
    }
    else
    {
      mRoutingPlanInplaceController.updateBuildProgress(progress, router);
    }
  }
}
