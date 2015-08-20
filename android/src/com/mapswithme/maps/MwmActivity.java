package com.mapswithme.maps;

import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Point;
import android.location.Location;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.AlertDialog;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.ImageButton;
import android.widget.Toast;

import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.DownloadActivity;
import com.mapswithme.country.DownloadFragment;
import com.mapswithme.country.StorageOptions;
import com.mapswithme.maps.Framework.OnBalloonListener;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.ads.LikesManager;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.ApiPoint;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.dialog.RoutingErrorDialogFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPredictor;
import com.mapswithme.maps.routing.RoutingResultCodesProcessor;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.search.SearchToolbarController;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.settings.StoragePathManager.MoveFilesListener;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.BottomButtonsLayout;
import com.mapswithme.maps.widget.FadeView;
import com.mapswithme.maps.widget.RoutingLayout;
import com.mapswithme.maps.widget.placepage.BasePlacePageAnimationController;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.Constants;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.io.Serializable;
import java.util.Stack;

public class MwmActivity extends BaseMwmFragmentActivity
    implements LocationHelper.LocationListener, OnBalloonListener, View.OnTouchListener, BasePlacePageAnimationController.OnVisibilityChangedListener,
    OnClickListener, Framework.RoutingListener, MapFragment.MapRenderingListener, CustomNavigateUpListener, Framework.RoutingProgressListener
{
  public static final String EXTRA_TASK = "map_task";
  private final static String TAG = "MwmActivity";
  private final static String EXTRA_CONSUMED = "mwm.extra.intent.processed";
  private final static String EXTRA_SCREENSHOTS_TASK = "screenshots_task";
  private final static String SCREENSHOTS_TASK_LOCATE = "locate_task";
  private final static String SCREENSHOTS_TASK_PPP = "show_place_page";
  private final static String EXTRA_LAT = "lat";
  private final static String EXTRA_LON = "lon";
  // Need it for change map style
  private static final String EXTRA_SET_MAP_STYLE = "set_map_style";
  // Instance state
  private static final String STATE_PP_OPENED = "PpOpened";
  private static final String STATE_MAP_OBJECT = "MapObject";
  private static final String STATE_BUTTONS_OPENED = "ButtonsOpened";

  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();
  private BroadcastReceiver mExternalStorageReceiver;
  private final StoragePathManager mPathManager = new StoragePathManager();
  private AlertDialog mStorageDisconnectedDialog;
  private ImageButton mBtnLocation;
  // map
  private MapFragment mMapFragment;
  // Place page
  private PlacePageView mPlacePage;
  // Routing
  private RoutingLayout mLayoutRouting;

  private boolean mNeedCheckUpdate = true;
  private int mLocationStateModeListenerId = LocationState.SLOT_UNDEFINED;
  // These flags are initialized to the invalid combination to force update on the first check
  // after launching.
  // These flags are static because the MwmActivity is recreated while screen orientation changing
  // but they shall not be reinitialized on screen orientation changing.
  private static boolean mStorageAvailable = false;
  private static boolean mStorageWritable = true;

  private FadeView mFadeView;

  private ViewGroup mNavigationButtons;
  private View mToolbarSearch;
  private ImageButton mBtnZoomIn;
  private ImageButton mBtnZoomOut;
  private BottomButtonsLayout mBottomButtons;

  private boolean mIsFragmentContainer;

  private LocationPredictor mLocationPredictor;
  private SearchToolbarController mSearchController;

  public static Intent createShowMapIntent(Context context, Index index, boolean doAutoDownload)
  {
    return new Intent(context, DownloadResourcesActivity.class)
        .putExtra(DownloadResourcesActivity.EXTRA_COUNTRY_INDEX, index)
        .putExtra(DownloadResourcesActivity.EXTRA_AUTODOWNLOAD_COUNTRY, doAutoDownload);
  }

  public static void setMapStyle(Context context, int mapStyle)
  {
    final Intent mapIntent = new Intent(context, MwmActivity.class);
    mapIntent.putExtra(EXTRA_SET_MAP_STYLE, mapStyle);
    context.startActivity(mapIntent);
    // Next we need to handle intent
  }

  public static void startSearch(Context context, String query)
  {
    final MwmActivity activity = (MwmActivity) context;
    if (activity.mIsFragmentContainer)
      activity.showSearch();
    else
      SearchActivity.startWithQuery(context, query);
  }

  public static Intent createUpdateMapsIntent()
  {
    return new Intent(MwmApplication.get(), DownloadResourcesActivity.class)
        .putExtra(DownloadResourcesActivity.EXTRA_UPDATE_COUNTRIES, true);
  }

  private void pauseLocation()
  {
    LocationHelper.INSTANCE.removeLocationListener(this);
    // Enable automatic turning screen off while app is idle
    Utils.keepScreenOn(false, getWindow());
    mLocationPredictor.pause();
  }

  private void listenLocationUpdates()
  {
    LocationHelper.INSTANCE.addLocationListener(this);
    // Do not turn off the screen while displaying position
    Utils.keepScreenOn(true, getWindow());
    mLocationPredictor.resume();
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

  private void checkUserMarkActivation()
  {
    final Intent intent = getIntent();
    if (intent != null && intent.hasExtra(EXTRA_SCREENSHOTS_TASK))
    {
      final String value = intent.getStringExtra(EXTRA_SCREENSHOTS_TASK);
      if (value.equals(SCREENSHOTS_TASK_PPP))
      {
        final double lat = Double.parseDouble(intent.getStringExtra(EXTRA_LAT));
        final double lon = Double.parseDouble(intent.getStringExtra(EXTRA_LON));
        mFadeView.getHandler().postDelayed(new Runnable()
        {
          @Override
          public void run()
          {
            Framework.nativeActivateUserMark(lat, lon);
          }
        }, 1000);
      }
    }
  }

  @Override
  public void onRenderingInitialized()
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        // Run all checks in main thread after rendering is initialized.
        checkMeasurementSystem();
        checkUpdateMapsWithoutSearchIndex();
        checkKitkatMigrationMove();
        checkLiteMapsInPro();
        checkUserMarkActivation();
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

  private void checkMeasurementSystem()
  {
    UnitLocale.initializeCurrentUnits();
  }

  private void checkKitkatMigrationMove()
  {
    mPathManager.checkKitkatMigration(this);
  }

  private void checkLiteMapsInPro()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT &&
        (Utils.isPackageInstalled(Constants.Package.MWM_LITE_PACKAGE) || Utils.isPackageInstalled(Constants.Package.MWM_SAMSUNG_PACKAGE)))
    {
      if (!mPathManager.containsLiteMapsOnSdcard())
        return;

      mPathManager.moveMapsLiteToPro(this,
          new MoveFilesListener()
          {
            @Override
            public void moveFilesFinished(String newPath)
            {
              UiUtils.showAlertDialog(MwmActivity.this, R.string.move_lite_maps_to_pro_ok);
            }

            @Override
            public void moveFilesFailed(int errorCode)
            {
              UiUtils.showAlertDialog(MwmActivity.this, R.string.move_lite_maps_to_pro_failed);
            }
          }
      );
    }
  }

  private void checkUpdateMapsWithoutSearchIndex()
  {
    // do it only once
    if (mNeedCheckUpdate)
    {
      mNeedCheckUpdate = false;

      MapStorage.INSTANCE.updateMapsWithoutSearchIndex(R.string.advise_update_maps, this, new MapStorage.UpdateFunctor()
      {
        @Override
        public void doUpdate()
        {
          runOnUiThread(new Runnable()
          {
            @Override
            public void run()
            {
              showDownloader(false);
            }
          });
        }

        @Override
        public void doCancel()
        {
        }
      });
    }
  }

  private void showBookmarks()
  {
    popAllFragments();
    startActivity(new Intent(this, BookmarkCategoriesActivity.class));
  }

  private void showSearchIfContainsSearchIndex()
  {
    if (!MapStorage.INSTANCE.updateMapsWithoutSearchIndex(R.string.search_update_maps, this, new MapStorage.UpdateFunctor()
    {
      @Override
      public void doUpdate()
      {
        showDownloader(false);
      }

      @Override
      public void doCancel()
      {
        showSearch();
      }
    }))
    {
      showSearch();
    }
  }

  private void showSearch()
  {
    if (mIsFragmentContainer)
    {
      if (getSupportFragmentManager().findFragmentByTag(SearchFragment.class.getName()) != null) // search is already shown
        return;
      hidePlacePage();
      Framework.deactivatePopup();
      popFragment();
      replaceFragment(SearchFragment.class.getName(), true, getIntent().getExtras());
    }
    else
      startActivity(new Intent(this, SearchActivity.class));
  }

  @Override
  public void replaceFragment(String fragmentClassName, boolean addToBackStack, Bundle args)
  {
    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, fragmentClassName, args);
    transaction.setCustomAnimations(R.anim.fragment_slide_in_bottom, R.anim.fragment_slide_out_bottom,
        R.anim.fragment_slide_in_bottom, R.anim.fragment_slide_out_bottom);
    transaction.replace(R.id.fragment_container, fragment, fragment.getClass().getName());
    if (addToBackStack)
      transaction.addToBackStack(null);

    transaction.commit();
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
    }
    else
    {
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
          })
          .create()
          .show();
    }
  }

  private void showDownloader(boolean openDownloadedList)
  {
    if (mIsFragmentContainer)
    {
      if (getSupportFragmentManager().findFragmentByTag(DownloadFragment.class.getName()) != null) // downloader is already shown
        return;
      popFragment();
      hidePlacePage();
      SearchToolbarController.cancelSearch();
      mSearchController.refreshToolbar();
      replaceFragment(DownloadFragment.class.getName(), true, new Bundle());
      mFadeView.fadeIn(false);
    }
    else
    {
      final Intent intent = new Intent(this, DownloadActivity.class).putExtra(DownloadActivity.EXTRA_OPEN_DOWNLOADED_LIST, openDownloadedList);
      startActivity(intent);
    }
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_map);
    initViews();

    TtsPlayer.INSTANCE.init();

    if (MwmApplication.get().nativeIsBenchmarking())
      Utils.keepScreenOn(true, getWindow());

    // TODO consider implementing other model of listeners connection, without activities being bound
    Framework.nativeSetRoutingListener(this);
    Framework.nativeSetRouteProgressListener(this);
    Framework.nativeSetBalloonListener(this);

    final Intent intent = getIntent();
    // We need check for tasks both in onCreate and onNewIntent
    // because of bug in OS: https://code.google.com/p/android/issues/detail?id=38629
    addTask(intent);

    if (intent != null && intent.hasExtra(EXTRA_SCREENSHOTS_TASK))
    {
      String value = intent.getStringExtra(EXTRA_SCREENSHOTS_TASK);
      if (value.equals(SCREENSHOTS_TASK_LOCATE))
        switchNextLocationState();
    }

    mLocationPredictor = new LocationPredictor(new Handler(), this);
    mSearchController = new SearchToolbarController(this);

    SharingHelper.prepare();
  }

  private void initViews()
  {
    initMap();
    initYota();
    initPlacePage();
    initRoutingBox();
    initNavigationButtons();
    if (findViewById(R.id.fragment_container) != null)
      mIsFragmentContainer = true;
  }

  private void initRoutingBox()
  {
    mLayoutRouting = (RoutingLayout) findViewById(R.id.layout__routing);
    mLayoutRouting.setListener(new RoutingLayout.ActionListener()
    {
      @Override
      public void onCloseRouting()
      {
        refreshZoomButtonsVisibility();
      }

      @Override
      public void onStartRouteFollow() {}

      @Override
      public void onRouteTypeChange(int type) {}
    });
  }

  private void initMap()
  {
    mFadeView = (FadeView) findViewById(R.id.fade_view);
    mFadeView.setFadeListener(new FadeView.FadeListener()
    {
      @Override
      public void onFadeOut()
      {
        if (mBottomButtons.areButtonsVisible())
          mBottomButtons.slideButtonsOut();
      }

      @Override
      public void onFadeIn()
      {

      }
    });
    mMapFragment = (MapFragment) getSupportFragmentManager().findFragmentByTag(MapFragment.FRAGMENT_TAG);
    if (mMapFragment == null)
    {
      mMapFragment = (MapFragment) MapFragment.instantiate(this, MapFragment.class.getName(), null);
      getSupportFragmentManager().beginTransaction().
          replace(R.id.map_fragment_container, mMapFragment, MapFragment.FRAGMENT_TAG).commit();
    }
    findViewById(R.id.map_fragment_container).setOnTouchListener(this);
  }

  @SuppressWarnings("deprecation")
  private void initNavigationButtons()
  {
    mBottomButtons = (BottomButtonsLayout) findViewById(R.id.map_bottom_buttons);
    mBottomButtons.hideButtons();
    mBottomButtons.findViewById(R.id.btn__open_menu).setOnClickListener(this);
    mBottomButtons.findViewById(R.id.ll__share).setOnClickListener(this);
    mBottomButtons.findViewById(R.id.ll__search).setOnClickListener(this);
    mBottomButtons.findViewById(R.id.ll__download_maps).setOnClickListener(this);
    mBottomButtons.findViewById(R.id.ll__bookmarks).setOnClickListener(this);
    mBottomButtons.findViewById(R.id.ll__settings).setOnClickListener(this);

    mNavigationButtons = (ViewGroup) findViewById(R.id.navigation_buttons);
    mBtnZoomIn = (ImageButton) mNavigationButtons.findViewById(R.id.map_button_plus);
    mBtnZoomIn.setOnClickListener(this);
    mBtnZoomOut = (ImageButton) mNavigationButtons.findViewById(R.id.map_button_minus);
    mBtnZoomOut.setOnClickListener(this);
    mBtnLocation = (ImageButton) mNavigationButtons.findViewById(R.id.btn__myposition);
    mBtnLocation.setOnClickListener(this);

    mToolbarSearch = findViewById(R.id.toolbar_search);
  }

  private void initPlacePage()
  {
    mPlacePage = (PlacePageView) findViewById(R.id.info_box);
    mPlacePage.setOnVisibilityChangedListener(this);
    mPlacePage.findViewById(R.id.ll__route).setOnClickListener(this);
  }

  private void initYota()
  {
    final View yopmeButton = findViewById(R.id.yop_it);
    if (Yota.isFirstYota())
      yopmeButton.setOnClickListener(this);
    else
      yopmeButton.setVisibility(View.INVISIBLE);
  }

  @Override
  public void onDestroy()
  {
    Framework.nativeRemoveBalloonListener();
    BottomSheetHelper.free();
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
    else if (mBottomButtons.areButtonsVisible())
      outState.putBoolean(STATE_BUTTONS_OPENED, true);

    super.onSaveInstanceState(outState);
  }

  @Override
  protected void onRestoreInstanceState(@NonNull Bundle savedInstanceState)
  {
    if (savedInstanceState.getBoolean(STATE_PP_OPENED))
    {
      mPlacePage.setMapObject((MapObject) savedInstanceState.getParcelable(STATE_MAP_OBJECT));
      mPlacePage.setState(State.PREVIEW);
    }

    if (savedInstanceState.getBoolean(STATE_BUTTONS_OPENED))
    {
      mFadeView.fadeInInstantly();
      mBottomButtons.showButtons();
    }

    super.onRestoreInstanceState(savedInstanceState);
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);

    if (intent != null)
    {
      if (intent.hasExtra(EXTRA_TASK))
        addTask(intent);
      else if (intent.hasExtra(EXTRA_SET_MAP_STYLE))
      {
        final int mapStyle = intent.getIntExtra(EXTRA_SET_MAP_STYLE, Framework.MAP_STYLE_LIGHT);
        Framework.setMapStyle(mapStyle);
      }
    }
  }

  private void addTask(Intent intent)
  {
    if (intent != null
        && !intent.getBooleanExtra(EXTRA_CONSUMED, false)
        && intent.hasExtra(EXTRA_TASK)
        && ((intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) == 0))
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

    // Notify user about turned off location services
    if (errorCode == LocationHelper.ERROR_DENIED)
    {
      LocationState.INSTANCE.turnOff();

      // Do not show this dialog on Kindle Fire - it doesn't have location services
      // and even wifi settings can't be opened programmatically
      if (!Utils.isAmazonDevice())
      {
        new AlertDialog.Builder(this).setTitle(R.string.location_is_disabled_long_text)
            .setPositiveButton(R.string.connection_settings, new DialogInterface.OnClickListener()
            {
              @Override
              public void onClick(DialogInterface dialog, int which)
              {
                try
                {
                  startActivity(new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS));
                } catch (final Exception e1)
                {
                  // On older Android devices location settings are merged with security
                  try
                  {
                    startActivity(new Intent(android.provider.Settings.ACTION_SECURITY_SETTINGS));
                  } catch (final Exception e2)
                  {
                    Log.w(TAG, "Can't run activity" + e2);
                  }
                }

                dialog.dismiss();
              }
            })
            .setNegativeButton(R.string.close, new DialogInterface.OnClickListener()
            {
              @Override
              public void onClick(DialogInterface dialog, int which)
              {
                dialog.dismiss();
              }
            })
            .create()
            .show();
      }
    }
    else if (errorCode == LocationHelper.ERROR_GPS_OFF)
    {
      Toast.makeText(this, R.string.gps_is_disabled_long_text, Toast.LENGTH_LONG).show();
    }
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    if (!l.getProvider().equals(LocationHelper.LOCATION_PREDICTOR_PROVIDER))
      mLocationPredictor.reset(l);

    mMapFragment.nativeLocationUpdated(
        l.getTime(),
        l.getLatitude(),
        l.getLongitude(),
        l.getAccuracy(),
        l.getAltitude(),
        l.getSpeed(),
        l.getBearing());

    if (mPlacePage.getState() != State.HIDDEN)
      mPlacePage.refreshLocation(l);

    RoutingLayout.State state = mLayoutRouting.getState();
    if (state != RoutingLayout.State.HIDDEN)
    {
      mLayoutRouting.updateRouteInfo();

      // TODO think about moving TtsPlayer logic to RoutingLayout to minimize native calls.
      if (state == RoutingLayout.State.TURN_INSTRUCTIONS)
      {
        final String[] turnNotifications = Framework.nativeGenerateTurnSound();
        if (turnNotifications != null)
          TtsPlayer.INSTANCE.speakNotifications(turnNotifications);
      }
    }
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    final int rotation = getWindowManager().getDefaultDisplay().getRotation();
    magneticNorth = LocationUtils.correctCompassAngle(rotation, magneticNorth);
    trueNorth = LocationUtils.correctCompassAngle(rotation, trueNorth);
    final double north = (trueNorth >= 0.0) ? trueNorth : magneticNorth;

    mMapFragment.nativeCompassUpdated(time, magneticNorth, trueNorth, accuracy);
    if (mPlacePage.getState() != State.HIDDEN)
      mPlacePage.refreshAzimuth(north);

    if (mLayoutRouting.getState() != RoutingLayout.State.HIDDEN)
      mLayoutRouting.refreshAzimuth(north);
  }

  // Callback from native location state mode element processing.
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
    LocationButtonImageSetter.setButtonViewFromState(newMode, mBtnLocation);
    switch (newMode)
    {
    case LocationState.UNKNOWN_POSITION:
      pauseLocation();
      break;
    case LocationState.PENDING_POSITION:
      listenLocationUpdates();
      break;
    default:
      break;
    }
  }

  private void listenLocationStateModeUpdates()
  {
    mLocationStateModeListenerId = LocationState.INSTANCE.addLocationStateModeListener(this);
  }

  private void stopWatchingCompassStatusUpdate()
  {
    LocationState.INSTANCE.removeLocationStateModeListener(mLocationStateModeListenerId);
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    listenLocationStateModeUpdates();
    invalidateLocationState();
    startWatchingExternalStorage();
    mSearchController.refreshToolbar();
    mPlacePage.onResume();
    LikesManager.INSTANCE.showDialogs(this);
    refreshZoomButtonsAfterLayout();
  }

  private void refreshZoomButtonsAfterLayout()
  {
    mFadeView.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener()
    {
      @SuppressWarnings("deprecation")
      @Override
      public void onGlobalLayout()
      {
        refreshZoomButtonsVisibility();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN)
          mFadeView.getViewTreeObserver().removeGlobalOnLayoutListener(this);
        else
          mFadeView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
      }
    });
  }

  private void refreshZoomButtonsVisibility()
  {
    final boolean showZoomSetting = MwmApplication.get().nativeGetBoolean(SettingsActivity.ZOOM_BUTTON_ENABLED, true) || Framework.nativeIsRoutingActive();
    UiUtils.showIf(showZoomSetting &&
            !UiUtils.areViewsIntersecting(mToolbarSearch, mBtnZoomIn) &&
            !UiUtils.areViewsIntersecting(mLayoutRouting, mBtnZoomIn),
        mBtnZoomIn, mBtnZoomOut);
  }

  @Override
  protected void onPause()
  {
    pauseLocation();
    stopWatchingExternalStorage();
    stopWatchingCompassStatusUpdate();
    TtsPlayer.INSTANCE.stop();
    LikesManager.INSTANCE.cancelDialogs();
    super.onPause();
  }

  private void updateExternalStorageState()
  {
    boolean available = false, writable = false;
    final String state = Environment.getExternalStorageState();
    if (Environment.MEDIA_MOUNTED.equals(state))
      available = writable = true;
    else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state))
      available = true;

    if (mStorageAvailable != available || mStorageWritable != writable)
    {
      mStorageAvailable = available;
      mStorageWritable = writable;
      handleExternalStorageState(available, writable);
    }
  }

  private void handleExternalStorageState(boolean available, boolean writeable)
  {
    if (available && writeable)
    {
      // Add local maps to the model
      mMapFragment.nativeStorageConnected();

      // @TODO enable downloader button and dismiss blocking popup

      if (mStorageDisconnectedDialog != null)
        mStorageDisconnectedDialog.dismiss();
    }
    else if (available)
    {
      // Add local maps to the model
      mMapFragment.nativeStorageConnected();

      // @TODO disable downloader button and dismiss blocking popup

      if (mStorageDisconnectedDialog != null)
        mStorageDisconnectedDialog.dismiss();
    }
    else
    {
      // Remove local maps from the model
      mMapFragment.nativeStorageDisconnected();

      // @TODO enable downloader button and show blocking popup

      if (mStorageDisconnectedDialog == null)
      {
        mStorageDisconnectedDialog = new AlertDialog.Builder(this)
            .setTitle(R.string.external_storage_is_not_available)
            .setMessage(getString(R.string.disconnect_usb_cable))
            .setCancelable(false)
            .create();
      }
      mStorageDisconnectedDialog.show();
    }
  }

  private void startWatchingExternalStorage()
  {
    mExternalStorageReceiver = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        updateExternalStorageState();
      }
    };

    registerReceiver(mExternalStorageReceiver, StoragePathManager.getMediaChangesIntentFilter());
    updateExternalStorageState();
  }

  private void stopWatchingExternalStorage()
  {
    mPathManager.stopExternalStorageWatching();
    if (mExternalStorageReceiver != null)
    {
      unregisterReceiver(mExternalStorageReceiver);
      mExternalStorageReceiver = null;
    }
  }

  @Override
  public void onBackPressed()
  {
    if (mPlacePage.getState() != State.HIDDEN)
    {
      hidePlacePage();
      Framework.deactivatePopup();
    }
    else if (mBottomButtons.areButtonsVisible())
    {
      mBottomButtons.toggle();
      mFadeView.fadeOut(false);
    }
    else if (canFragmentInterceptBackPress())
      return;
    else if (popFragment())
    {
      InputUtils.hideKeyboard(mFadeView);
      mFadeView.fadeOut(false);
    }
    else
      super.onBackPressed();
  }

  private boolean isMapFaded()
  {
    return mFadeView.getVisibility() == View.VISIBLE;
  }

  private boolean canFragmentInterceptBackPress()
  {
    final FragmentManager manager = getSupportFragmentManager();
    DownloadFragment fragment = (DownloadFragment) manager.findFragmentByTag(DownloadFragment.class.getName());
    return fragment != null && fragment.isResumed() && fragment.onBackPressed();
  }

  private boolean popFragment()
  {
    for (String tag : new String[]{SearchFragment.class.getName(), DownloadFragment.class.getName()})
      if (popFragment(tag))
        return true;

    return false;
  }

  private void popAllFragments()
  {
    for (String tag : new String[]{SearchFragment.class.getName(), DownloadFragment.class.getName()})
      popFragment(tag);
  }

  private boolean popFragment(String className)
  {
    final FragmentManager manager = getSupportFragmentManager();
    Fragment fragment = manager.findFragmentByTag(className);
    // TODO d.yunitsky
    // we cant pop fragment, if it isn't resumed, cause of 'at android.support.v4.app.FragmentManagerImpl.checkStateLoss(FragmentManager.java:1375)'
    if (fragment != null && fragment.isResumed())
    {
      manager.popBackStackImmediate();
      return true;
    }

    return false;
  }

  // Callbacks from native map objects touch event.
  @Override
  public void onApiPointActivated(final double lat, final double lon, final String name, final String id)
  {
    if (ParsedMwmRequest.hasRequest())
    {
      final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
      request.setPointData(lat, lon, name, id);

      runOnUiThread(new Runnable()
      {
        @Override
        public void run()
        {
          final String poiType = ParsedMwmRequest.getCurrentRequest().getCallerName(MwmApplication.get()).toString();
          activateMapObject(new ApiPoint(name, id, poiType, lat, lon));
        }
      });
    }
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
    final MapObject mypos = new MapObject.MyPosition(getString(R.string.my_position), lat, lon);

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
    mPlacePage.bringToFront();
    if (!mPlacePage.hasMapObject(object))
    {
      mPlacePage.setMapObject(object);
      mPlacePage.setState(State.PREVIEW);
      popAllFragments();
      if (isMapFaded())
        mFadeView.fadeOut(false);
    }
  }

  private void hidePlacePage()
  {
    mPlacePage.setState(State.HIDDEN);
    mPlacePage.setMapObject(null);
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
          hidePlacePage();
          Framework.deactivatePopup();
        }
      });
    }
  }

  @Override
  public void onPreviewVisibilityChanged(boolean isVisible)
  {
    if (isVisible)
    {
      if (previewIntersectsBottomMenu())
        mBottomButtons.setVisibility(View.GONE);
      if (previewIntersectsZoomButtons())
        UiUtils.hide(mBtnZoomIn, mBtnZoomOut);
    }
    else
    {
      Framework.deactivatePopup();
      mPlacePage.setMapObject(null);
      refreshZoomButtonsVisibility();
      mBottomButtons.setVisibility(View.VISIBLE);
    }
  }

  private boolean previewIntersectsBottomMenu()
  {
    return !(UiUtils.isBigTablet() || (UiUtils.isSmallTablet() && getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE));
  }

  private boolean previewIntersectsZoomButtons()
  {
    return !(UiUtils.isBigTablet() || UiUtils.isSmallTablet());
  }

  @Override
  public void onPlacePageVisibilityChanged(boolean isVisible)
  {
    if (isVisible)
    {
      AlohaHelper.logClick(AlohaHelper.PP_OPEN);
      if (placePageIntersectsZoomButtons())
        UiUtils.hide(mBtnZoomIn, mBtnZoomOut);
      else
        refreshZoomButtonsVisibility();
    }
    else
      AlohaHelper.logClick(AlohaHelper.PP_CLOSE);
  }

  private boolean placePageIntersectsZoomButtons()
  {
    return !(UiUtils.isBigTablet() || (UiUtils.isSmallTablet() && getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE));
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.ll__share:
      AlohaHelper.logClick(AlohaHelper.MENU_SHARE);
      shareMyLocation();
      mBottomButtons.hideButtons();
      UiUtils.hide(mFadeView);
      UiUtils.clearAnimationAfterAlpha(mFadeView);
      break;
    case R.id.ll__settings:
      AlohaHelper.logClick(AlohaHelper.MENU_SETTINGS);
      startActivity(new Intent(this, SettingsActivity.class));
      mBottomButtons.hideButtons();
      UiUtils.hide(mFadeView);
      UiUtils.clearAnimationAfterAlpha(mFadeView);
      break;
    case R.id.ll__download_maps:
      AlohaHelper.logClick(AlohaHelper.MENU_DOWNLOADER);
      showDownloader(false);
      mBottomButtons.hideButtons();
      UiUtils.hide(mFadeView);
      UiUtils.clearAnimationAfterAlpha(mFadeView);
      break;
    case R.id.ll__route:
      AlohaHelper.logClick(AlohaHelper.PP_ROUTE);
      mLayoutRouting.setEndPoint(mPlacePage.getMapObject());
      mLayoutRouting.setState(RoutingLayout.State.PREPARING, true);
      mPlacePage.setState(PlacePageView.State.HIDDEN);
      break;
    case R.id.map_button_plus:
      AlohaHelper.logClick(AlohaHelper.ZOOM_IN);
      mMapFragment.nativeScale(3.0 / 2);
      break;
    case R.id.map_button_minus:
      AlohaHelper.logClick(AlohaHelper.ZOOM_OUT);
      mMapFragment.nativeScale(2 / 3.0);
      break;
    case R.id.btn__open_menu:
      AlohaHelper.logClick(AlohaHelper.TOOLBAR_MENU);
      mFadeView.fadeIn(false);
      mBottomButtons.toggle();
      break;
    case R.id.ll__search:
      AlohaHelper.logClick(AlohaHelper.TOOLBAR_SEARCH);
      showSearchIfContainsSearchIndex();
      mBottomButtons.hideButtons();
      UiUtils.hide(mFadeView);
      UiUtils.clearAnimationAfterAlpha(mFadeView);
      break;
    case R.id.ll__bookmarks:
      AlohaHelper.logClick(AlohaHelper.TOOLBAR_BOOKMARKS);
      showBookmarks();
      mBottomButtons.hideButtons();
      UiUtils.hide(mFadeView);
      UiUtils.clearAnimationAfterAlpha(mFadeView);
      break;
    case R.id.btn__myposition:
      switchNextLocationState();
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
    default:
      break;
    }
  }

  private void closeRouting()
  {

    mLayoutRouting.setState(RoutingLayout.State.HIDDEN, true);
  }

  private static void switchNextLocationState()
  {
    LocationState.INSTANCE.switchToNextMode();
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    boolean result = false;
    if (mPlacePage.getState() == State.DETAILS || mPlacePage.getState() == State.BOOKMARK)
    {
      Framework.deactivatePopup();
      hidePlacePage();
      result = true;
    }

    return result || mMapFragment.onTouch(view, event);
  }

  @Override
  public boolean onKeyUp(int keyCode, @NonNull KeyEvent event)
  {
    if (keyCode == KeyEvent.KEYCODE_MENU)
    {
      if (mBottomButtons.areButtonsVisible())
        mFadeView.fadeOut(false);
      else
        mFadeView.fadeIn(false);
      mBottomButtons.toggle();
      return true;
    }
    return super.onKeyUp(keyCode, event);
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (resultCode == RESULT_OK && requestCode == ChooseBookmarkCategoryActivity.REQUEST_CODE_BOOKMARK_SET)
    {
      final Point bookmarkAndCategory = ((ParcelablePoint) data.getParcelableExtra(ChooseBookmarkCategoryActivity.BOOKMARK)).getPoint();
      final Bookmark bookmark = BookmarkManager.INSTANCE.getBookmark(bookmarkAndCategory.x, bookmarkAndCategory.y);
      mPlacePage.setMapObject(bookmark);
    }
    super.onActivityResult(requestCode, resultCode, data);
  }

  @Override
  public void onRoutingEvent(final int resultCode, final Index[] missingCountries)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        if (resultCode == RoutingResultCodesProcessor.NO_ERROR)
        {
          if (Framework.getRouter() == Framework.ROUTER_TYPE_PEDESTRIAN)
            LikesManager.INSTANCE.onPedestrianBuilt();

          mLayoutRouting.setState(RoutingLayout.State.ROUTE_BUILT, true);
        }
        else
        {
          mLayoutRouting.setState(RoutingLayout.State.ROUTE_BUILD_ERROR, true);
          final Bundle args = new Bundle();
          args.putInt(RoutingErrorDialogFragment.EXTRA_RESULT_CODE, resultCode);
          args.putSerializable(RoutingErrorDialogFragment.EXTRA_MISSING_COUNTRIES, missingCountries);
          final RoutingErrorDialogFragment fragment = (RoutingErrorDialogFragment) Fragment.instantiate(MwmActivity.this, RoutingErrorDialogFragment.class.getName());
          fragment.setArguments(args);
          fragment.setListener(new RoutingErrorDialogFragment.RoutingDialogListener()
          {
            @Override
            public void onDownload()
            {
              mLayoutRouting.setState(RoutingLayout.State.HIDDEN, false);
              refreshZoomButtonsVisibility();
              ActiveCountryTree.downloadMapsForIndex(missingCountries, StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING);
              showDownloader(true);
            }

            @Override
            public void onCancel()
            {
              refreshZoomButtonsVisibility();
            }

            @Override
            public void onOk()
            {
              if (RoutingResultCodesProcessor.isDownloadable(resultCode))
              {
                mLayoutRouting.setState(RoutingLayout.State.HIDDEN, false);
                refreshZoomButtonsVisibility();
                showDownloader(false);
              }
            }
          });
          fragment.show(getSupportFragmentManager(), RoutingErrorDialogFragment.class.getName());
        }

        refreshZoomButtonsVisibility();
      }
    });
  }

  @Override
  public void onRouteBuildingProgress(final float progress)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        mLayoutRouting.setRouteBuildingProgress(progress);
      }
    });
  }

  @Override
  public void customOnNavigateUp()
  {
    if (popFragment())
    {
      InputUtils.hideKeyboard(mBottomButtons);
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

  public static class UpdateCountryTask implements MapTask
  {
    @Override
    public boolean run(final MwmActivity target)
    {
      target.runOnUiThread(new Runnable()
      {
        @Override
        public void run()
        {
          target.showDownloader(true);
        }
      });
      return true;
    }
  }
}
