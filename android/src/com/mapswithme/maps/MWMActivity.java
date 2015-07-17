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
import android.support.v4.view.ViewCompat;
import android.support.v7.app.AlertDialog;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.AbsoluteSizeSpan;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.DownloadActivity;
import com.mapswithme.country.DownloadFragment;
import com.mapswithme.country.StorageOptions;
import com.mapswithme.maps.Framework.OnBalloonListener;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.ads.LikesManager;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.ChooseBookmarkCategoryActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.ApiPoint;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.data.RouterTypes;
import com.mapswithme.maps.data.RoutingResultCodesProcessor;
import com.mapswithme.maps.dialog.RoutingErrorDialogFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPredictor;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchToolbarController;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.settings.StoragePathManager.SetStoragePathListener;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.widget.BottomButtonsLayout;
import com.mapswithme.maps.widget.FadeView;
import com.mapswithme.maps.widget.placepage.BasePlacePageAnimationController;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.Constants;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.sharing.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.view.ViewHelper;

import java.io.Serializable;
import java.util.Stack;
import java.util.concurrent.TimeUnit;

public class MWMActivity extends BaseMwmFragmentActivity
    implements LocationHelper.LocationListener, OnBalloonListener, View.OnTouchListener, BasePlacePageAnimationController.OnVisibilityChangedListener,
    OnClickListener, Framework.RoutingListener, MapFragment.MapRenderingListener, CustomNavigateUpListener
{
  public static final String EXTRA_TASK = "map_task";
  private final static String TAG = "MWMActivity";
  private final static String EXTRA_CONSUMED = "mwm.extra.intent.processed";
  private final static String EXTRA_SCREENSHOTS_TASK = "screenshots_task";
  private final static String SCREENSHOTS_TASK_LOCATE = "locate_task";
  private final static String SCREENSHOTS_TASK_PPP = "show_place_page";
  private final static String EXTRA_LAT = "lat";
  private final static String EXTRA_LON = "lon";
  private final static String EXTRA_COUNTRY_INDEX = "country_index";
  // Need it for search
  private static final String EXTRA_SEARCH_RES_SINGLE = "search_res_index";
  // Need it for change map style
  private static final String EXTRA_SET_MAP_STYLE = "set_map_style";
  // Instance state
  private static final String STATE_ROUTE_FOLLOWED = "RouteFollowed";
  private static final String STATE_PP_OPENED = "PpOpened";
  private static final String STATE_MAP_OBJECT = "MapObject";
  private static final String STATE_BUTTONS_OPENED = "ButtonsOpened";
  private static final int BASE_ANIM_DURATION = 30;
  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();
  private BroadcastReceiver mExternalStorageReceiver;
  private StoragePathManager mPathManager = new StoragePathManager();
  private AlertDialog mStorageDisconnectedDialog;
  private ImageButton mBtnLocation;
  // map
  private MapFragment mMapFragment;
  // Place page
  private PlacePageView mPlacePage;
  private View mRlStartRouting;
  private ProgressBar mPbRoutingProgress;
  private TextView mTvStartRouting;
  private ImageView mIvStartRouting;
  // Routing
  private TextView mTvRoutingDistance;
  private RelativeLayout mRlRoutingBox;
  private RelativeLayout mLayoutRoutingGo;
  private RelativeLayout mRlTurnByTurnBox;
  private TextView mTvTotalDistance;
  private TextView mTvTotalTime;
  private ImageView mIvTurn;
  private TextView mTvTurnDistance;

  private boolean mNeedCheckUpdate = true;
  private int mLocationStateModeListenerId = LocationState.SLOT_UNDEFINED;
  // These flags are initialized to the invalid combination to force update on the first check
  // after launching.
  // These flags are static because the MWMActivity is recreated while screen orientation changing
  // but they shall not be reinitialized on screen orientation changing.
  private static boolean mStorageAvailable = false;
  private static boolean mStorageWritable = true;

  private FadeView mFadeView;

  private ViewGroup mNavigationButtons;
  private View mToolbarSearch;
  private ImageButton mBtnZoomIn;
  private ImageButton mBtnZoomOut;
  private BottomButtonsLayout mBottomButtons;

  private static final String IS_KML_MOVED = "KmlBeenMoved";
  private static final String IS_KITKAT_MIGRATION_COMPLETED = "KitKatMigrationCompleted";
  // for routing
  private static final String IS_ROUTING_DISCLAIMER_APPROVED = "IsDisclaimerApproved";

  private boolean mIsFragmentContainer;

  private LocationPredictor mLocationPredictor;
  private LikesManager mLikesManager;
  private SearchToolbarController mSearchController;

  public static Intent createShowMapIntent(Context context, Index index, boolean doAutoDownload)
  {
    return new Intent(context, DownloadResourcesActivity.class)
        .putExtra(DownloadResourcesActivity.EXTRA_COUNTRY_INDEX, index)
        .putExtra(DownloadResourcesActivity.EXTRA_AUTODOWNLOAD_CONTRY, doAutoDownload);
  }

  public static void startWithSearchResult(Context context, boolean single)
  {
    final Intent mapIntent = new Intent(context, MWMActivity.class);
    mapIntent.putExtra(EXTRA_SEARCH_RES_SINGLE, single);
    context.startActivity(mapIntent);
    // Next we need to handle intent
  }

  public static void setMapStyle(Context context, int mapStyle)
  {
    final Intent mapIntent = new Intent(context, MWMActivity.class);
    mapIntent.putExtra(EXTRA_SET_MAP_STYLE, mapStyle);
    context.startActivity(mapIntent);
    // Next we need to handle intent
  }

  public static void startSearch(Context context, String query)
  {
    final MWMActivity activity = (MWMActivity) context;
    if (activity.mIsFragmentContainer)
      activity.showSearch();
    else
      SearchActivity.startWithQuery(context, query);
  }

  public static Intent createUpdateMapsIntent()
  {
    return new Intent(MWMApplication.get(), DownloadResourcesActivity.class)
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
        checkUpdateMaps();
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
    final boolean kmlMoved = MWMApplication.get().nativeGetBoolean(IS_KML_MOVED, false);
    final boolean mapsCpy = MWMApplication.get().nativeGetBoolean(IS_KITKAT_MIGRATION_COMPLETED, false);

    if (!kmlMoved)
      if (mPathManager.moveBookmarks())
        MWMApplication.get().nativeSetBoolean(IS_KML_MOVED, true);
      else
      {
        UiUtils.showAlertDialog(this, R.string.bookmark_move_fail);
        return;
      }

    if (!mapsCpy)
      mPathManager.checkWritableDir(this,
          new SetStoragePathListener()
          {
            @Override
            public void moveFilesFinished(String newPath)
            {
              MWMApplication.get().nativeSetBoolean(IS_KITKAT_MIGRATION_COMPLETED, true);
              UiUtils.showAlertDialog(MWMActivity.this, R.string.kitkat_migrate_ok);
            }

            @Override
            public void moveFilesFailed(int errorCode)
            {
              UiUtils.showAlertDialog(MWMActivity.this, R.string.kitkat_migrate_failed);
            }
          }
      );
  }

  private void checkLiteMapsInPro()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT &&
        (Utils.isPackageInstalled(Constants.Package.MWM_LITE_PACKAGE) || Utils.isPackageInstalled(Constants.Package.MWM_SAMSUNG_PACKAGE)))
    {
      if (!mPathManager.containsLiteMapsOnSdcard())
        return;

      mPathManager.moveMapsLiteToPro(this,
          new SetStoragePathListener()
          {
            @Override
            public void moveFilesFinished(String newPath)
            {
              UiUtils.showAlertDialog(MWMActivity.this, R.string.move_lite_maps_to_pro_ok);
            }

            @Override
            public void moveFilesFailed(int errorCode)
            {
              UiUtils.showAlertDialog(MWMActivity.this, R.string.move_lite_maps_to_pro_failed);
            }
          }
      );
    }
  }

  private void checkUpdateMaps()
  {
    // do it only once
    if (mNeedCheckUpdate)
    {
      mNeedCheckUpdate = false;

      MapStorage.INSTANCE.updateMaps(R.string.advise_update_maps, this, new MapStorage.UpdateFunctor()
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
    // TODO open in fragment?
    startActivity(new Intent(this, BookmarkCategoriesActivity.class));
  }

  private void showSearchIfUpdated()
  {
    if (!MapStorage.INSTANCE.updateMaps(R.string.search_update_maps, this, new MapStorage.UpdateFunctor()
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
      // we use shortest message we can have here
      ShareAction.AnyShareAction.share(this, body);
    }
    else
    {
      new AlertDialog.Builder(MWMActivity.this)
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

    // Do not turn off the screen while benchmarking
    if (MWMApplication.get().nativeIsBenchmarking())
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    Framework.nativeSetRoutingListener(this);
    Framework.nativeConnectBalloonListeners(this);

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
    mLikesManager = new LikesManager(this);
    mSearchController = new SearchToolbarController(this);
    restoreRoutingState(savedInstanceState);

    SharingHelper.prepare();
  }

  private void restoreRoutingState(@Nullable Bundle savedInstanceState)
  {
    if (Framework.nativeIsRoutingActive())
    {
      if (savedInstanceState != null && savedInstanceState.getBoolean(STATE_ROUTE_FOLLOWED))
      {
        updateRoutingDistance();
        mRlTurnByTurnBox.setVisibility(View.VISIBLE);
        mRlRoutingBox.setVisibility(View.GONE);
      }
      else if (Framework.nativeIsRouteBuilt())
      {
        updateRoutingDistance();
        mRlRoutingBox.setVisibility(View.VISIBLE);
        mRlTurnByTurnBox.setVisibility(View.GONE);
      }
      else if (savedInstanceState != null)
      {
        final MapObject object = savedInstanceState.getParcelable(STATE_MAP_OBJECT);
        if (object != null)
        {
          mPlacePage.setState(State.PREVIEW);
          mPlacePage.setMapObject(object);
        }
        mIvStartRouting.setVisibility(View.GONE);
        mTvStartRouting.setVisibility(View.GONE);
        mPbRoutingProgress.setVisibility(View.VISIBLE);
      }
    }
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
    mRlStartRouting = mPlacePage.findViewById(R.id.rl__route);
    mRlStartRouting.setOnClickListener(this);
    mTvStartRouting = (TextView) mRlStartRouting.findViewById(R.id.tv__route);
    mIvStartRouting = (ImageView) mRlStartRouting.findViewById(R.id.iv__route);
    mPbRoutingProgress = (ProgressBar) mRlStartRouting.findViewById(R.id.pb__routing_progress);
  }

  private void initRoutingBox()
  {
    mRlRoutingBox = (RelativeLayout) findViewById(R.id.rl__routing_box);
    mRlRoutingBox.setVisibility(View.GONE);
    mRlRoutingBox.findViewById(R.id.iv__routing_close).setOnClickListener(this);
    mLayoutRoutingGo = (RelativeLayout) mRlRoutingBox.findViewById(R.id.rl__routing_go);
    mLayoutRoutingGo.setOnClickListener(this);
    mTvRoutingDistance = (TextView) mRlRoutingBox.findViewById(R.id.tv__routing_distance);

    mRlTurnByTurnBox = (RelativeLayout) findViewById(R.id.layout__turn_instructions);
    mTvTotalDistance = (TextView) mRlTurnByTurnBox.findViewById(R.id.tv__total_distance);
    mTvTotalTime = (TextView) mRlTurnByTurnBox.findViewById(R.id.tv__total_time);
    mIvTurn = (ImageView) mRlTurnByTurnBox.findViewById(R.id.iv__turn);
    mTvTurnDistance = (TextView) mRlTurnByTurnBox.findViewById(R.id.tv__turn_distance);
    mRlTurnByTurnBox.findViewById(R.id.btn__close).setOnClickListener(this);
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
    Framework.nativeClearBalloonListeners();
    BottomSheetHelper.free();
    super.onDestroy();
  }

  @Override
  protected void onSaveInstanceState(Bundle outState)
  {
    if (mRlTurnByTurnBox.getVisibility() == View.VISIBLE)
      outState.putBoolean(STATE_ROUTE_FOLLOWED, true);
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
      if (mPlacePage.getMapObject() instanceof MapObject.MyPosition)
        mRlStartRouting.setVisibility(View.GONE);
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
      else if (intent.hasExtra(EXTRA_SEARCH_RES_SINGLE))
      {
        if (intent.getBooleanExtra(EXTRA_SEARCH_RES_SINGLE, false))
        {
          popFragment();
          MapObject.SearchResult result = new MapObject.SearchResult(0);
          activateMapObject(result);
        }
        else
          onDismiss();
      }
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

    updateRoutingDistance();
  }

  private void updateRoutingDistance()
  {
    final LocationState.RoutingInfo info = Framework.nativeGetRouteFollowingInfo();
    if (info != null)
    {
      SpannableStringBuilder builder = new SpannableStringBuilder(info.mDistToTarget).append(" ").append(info.mUnits.toUpperCase());
      builder.setSpan(new AbsoluteSizeSpan(34, true), 0, info.mDistToTarget.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      builder.setSpan(new AbsoluteSizeSpan(10, true), info.mDistToTarget.length(), builder.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      mTvRoutingDistance.setText(builder);
      builder.setSpan(new AbsoluteSizeSpan(25, true), 0, info.mDistToTarget.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      builder.setSpan(new AbsoluteSizeSpan(14, true), info.mDistToTarget.length(), builder.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      mTvTotalDistance.setText(builder);
      mIvTurn.setImageResource(getTurnImageResource(info));
      if (LocationState.RoutingInfo.TurnDirection.isLeftTurn(info.mTurnDirection))
        ViewHelper.setScaleX(mIvTurn, -1); // right turns are displayed as mirrored left turns.
      else
        ViewHelper.setScaleX(mIvTurn, 1);

      // one minute is added to estimated time to destination point
      // to prevent displaying that zero minutes are left to the finish near destination point
      final long minutes = TimeUnit.SECONDS.toMinutes(info.mTotalTimeInSeconds) + 1;
      final long hours = TimeUnit.MINUTES.toHours(minutes);
      final String time = String.format("%d:%02d", hours, minutes - TimeUnit.HOURS.toMinutes(hours));
      mTvTotalTime.setText(time);

      builder = new SpannableStringBuilder(info.mDistToTurn).append(" ").append(info.mTurnUnitsSuffix.toUpperCase());
      builder.setSpan(new AbsoluteSizeSpan(44, true), 0, info.mDistToTurn.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      builder.setSpan(new AbsoluteSizeSpan(11, true), info.mDistToTurn.length(), builder.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      mTvTurnDistance.setText(builder);
    }
  }

  private int getTurnImageResource(LocationState.RoutingInfo info)
  {
    switch (info.mTurnDirection)
    {
    case NO_TURN:
    case GO_STRAIGHT:
      return R.drawable.ic_straight_compact;
    case TURN_RIGHT:
      return R.drawable.ic_simple_compact;
    case TURN_SHARP_RIGHT:
      return R.drawable.ic_sharp_compact;
    case TURN_SLIGHT_RIGHT:
      return R.drawable.ic_slight_compact;
    case TURN_LEFT:
      return R.drawable.ic_simple_compact;
    case TURN_SHARP_LEFT:
      return R.drawable.ic_sharp_compact;
    case TURN_SLIGHT_LEFT:
      return R.drawable.ic_slight_compact;
    case U_TURN:
      return R.drawable.ic_uturn_compact;
    case ENTER_ROUND_ABOUT:
    case LEAVE_ROUND_ABOUT:
    case STAY_ON_ROUND_ABOUT:
      return R.drawable.ic_round_compact;
    }

    return 0;
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
    refreshRouterIcon();
    mSearchController.refreshToolbar();
    mPlacePage.onResume();
    mLikesManager.showLikeDialogForCurrentSession();
    refreshZoomButtonsAfterLayout();
  }

  private void refreshRouterIcon()
  {
    if (RouterTypes.getRouterType().equals(RouterTypes.ROUTER_VEHICLE))
      mIvStartRouting.setImageResource(R.drawable.ic_route);
    else
      mIvStartRouting.setImageResource(R.drawable.ic_walk);
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
    final boolean showZoomSetting = MWMApplication.get().nativeGetBoolean(SettingsActivity.ZOOM_BUTTON_ENABLED, true) || Framework.nativeIsRoutingActive();
    UiUtils.showIf(showZoomSetting &&
            !UiUtils.areViewsIntersecting(mToolbarSearch, mBtnZoomIn) &&
            !UiUtils.areViewsIntersecting(mRlRoutingBox, mBtnZoomIn),
        mBtnZoomIn, mBtnZoomOut);
  }

  @Override
  protected void onPause()
  {
    pauseLocation();
    stopWatchingExternalStorage();
    stopWatchingCompassStatusUpdate();
    super.onPause();
    mLikesManager.cancelLikeDialog();
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
    final FragmentManager manager = getSupportFragmentManager();
    for (String tag : new String[]{SearchFragment.class.getName(), DownloadFragment.class.getName()})
    {
      Fragment fragment = manager.findFragmentByTag(tag);
      // TODO we cant pop fragment, if it isn't resumed, cause of 'at android.support.v4.app.FragmentManagerImpl.checkStateLoss(FragmentManager.java:1375)'
      // consider other possibilities here
      if (fragment != null && fragment.isResumed())
      {
        manager.popBackStackImmediate();
        return true;
      }
    }

    return false;
  }

  private void popAllFragments()
  {
    final FragmentManager manager = getSupportFragmentManager();
    while (manager.getBackStackEntryCount() > 0)
      manager.popBackStackImmediate();
  }

  // Callbacks from native map objects touch event.
  @Override
  public void onApiPointActivated(final double lat, final double lon, final String name, final String id)
  {
    if (ParsedMmwRequest.hasRequest())
    {
      final ParsedMmwRequest request = ParsedMmwRequest.getCurrentRequest();
      request.setPointData(lat, lon, name, id);

      runOnUiThread(new Runnable()
      {
        @Override
        public void run()
        {
          final String poiType = ParsedMmwRequest.getCurrentRequest().getCallerName(MWMApplication.get()).toString();
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
          mRlStartRouting.setVisibility(View.GONE);
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
      mRlStartRouting.setVisibility(View.VISIBLE);
      mTvStartRouting.setVisibility(View.VISIBLE);
      mIvStartRouting.setVisibility(View.VISIBLE);
      mPbRoutingProgress.setVisibility(View.GONE);
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
    case R.id.rl__route:
      AlohaHelper.logClick(AlohaHelper.PP_ROUTE);
      buildRoute();
      break;
    case R.id.iv__routing_close:
      AlohaHelper.logClick(AlohaHelper.ROUTING_GO_CLOSE);
      closeRouting();
      break;
    case R.id.btn__close:
      AlohaHelper.logClick(AlohaHelper.ROUTING_CLOSE);
      closeRouting();
      break;
    case R.id.rl__routing_go:
      AlohaHelper.logClick(AlohaHelper.ROUTING_GO);
      followRoute();
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
      showSearchIfUpdated();
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

  private void followRoute()
  {
    Framework.nativeFollowRoute();

    Animator animator = ObjectAnimator.ofFloat(mRlRoutingBox, "alpha", 1, 0);
    animator.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        mRlTurnByTurnBox.setVisibility(View.VISIBLE);
        mRlRoutingBox.setVisibility(View.GONE);
      }
    });
    animator.start();
  }

  private void buildRoute()
  {
    if (!MWMApplication.get().nativeGetBoolean(IS_ROUTING_DISCLAIMER_APPROVED, false))
    {
      showRoutingDisclaimer();
      return;
    }

    final MapObject mapObject = mPlacePage.getMapObject();
    if (mapObject != null)
    {
      mIvStartRouting.setVisibility(View.GONE);
      mTvStartRouting.setVisibility(View.GONE);
      mPbRoutingProgress.setVisibility(View.VISIBLE);
      Framework.nativeBuildRoute(mapObject.getLat(), mapObject.getLon());
    }
    else
      Log.d(MWMActivity.class.getName(), "buildRoute(). MapObject is null. MapInfoView visibility : " + mPlacePage.getVisibility());
  }

  private void showRoutingDisclaimer()
  {
    StringBuilder builder = new StringBuilder();
    for (int resId : new int[] {R.string.dialog_routing_disclaimer_priority, R.string.dialog_routing_disclaimer_precision,
        R.string.dialog_routing_disclaimer_recommendations, R.string.dialog_routing_disclaimer_beware})
      builder.append(getString(resId)).append("\n\n");

    new AlertDialog.Builder(this)
        .setTitle(R.string.dialog_routing_disclaimer_title)
        .setMessage(builder.toString())
        .setCancelable(false)
        .setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            MWMApplication.get().nativeSetBoolean(IS_ROUTING_DISCLAIMER_APPROVED, true);
            dlg.dismiss();
            buildRoute();
          }
        })
        .setNegativeButton(getString(R.string.cancel), new DialogInterface.OnClickListener()
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

  private void closeRouting()
  {
    mPlacePage.bringToFront();
    mIvStartRouting.setVisibility(View.VISIBLE);
    mTvStartRouting.setVisibility(View.VISIBLE);
    mPbRoutingProgress.setVisibility(View.GONE);
    mRlRoutingBox.clearAnimation();
    UiUtils.hide(mRlRoutingBox, mPbRoutingProgress, mRlTurnByTurnBox);
    mRlStartRouting.setVisibility(View.VISIBLE);

    Framework.nativeCloseRouting();
    refreshZoomButtonsVisibility();
  }

  private void switchNextLocationState()
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
    if (resultCode == RESULT_OK)
    {
      if (requestCode == ChooseBookmarkCategoryActivity.REQUEST_CODE_EDIT_BOOKMARK)
      {
        final Point bmk = ((ParcelablePoint) data.getParcelableExtra(ChooseBookmarkCategoryActivity.BOOKMARK)).getPoint();
        onBookmarkActivated(bmk.x, bmk.y);
      }
      else if (requestCode == ChooseBookmarkCategoryActivity.REQUEST_CODE_SET)
      {
        final Point pin = ((ParcelablePoint) data.getParcelableExtra(ChooseBookmarkCategoryActivity.BOOKMARK)).getPoint();
        final Bookmark bookmark = BookmarkManager.INSTANCE.getBookmark(pin.x, pin.y);
        mPlacePage.setMapObject(bookmark);
      }
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
          mRlTurnByTurnBox.setVisibility(View.GONE);
          ViewCompat.setAlpha(mLayoutRoutingGo, 1);
          mLayoutRoutingGo.setVisibility(View.VISIBLE);

          Animator animator = ObjectAnimator.ofFloat(mRlRoutingBox, "alpha", 0, 1);
          animator.setDuration(BASE_ANIM_DURATION);
          animator.start();

          mRlRoutingBox.setVisibility(View.VISIBLE);
          mRlRoutingBox.bringToFront();

          hidePlacePage();
          Framework.deactivatePopup();
          updateRoutingDistance();
        }
        else
        {
          final Bundle args = new Bundle();
          args.putInt(RoutingErrorDialogFragment.EXTRA_RESULT_CODE, resultCode);
          args.putSerializable(RoutingErrorDialogFragment.EXTRA_MISSING_COUNTRIES, missingCountries);
          final RoutingErrorDialogFragment fragment = (RoutingErrorDialogFragment) Fragment.instantiate(MWMActivity.this, RoutingErrorDialogFragment.class.getName());
          fragment.setArguments(args);
          fragment.setListener(new RoutingErrorDialogFragment.RoutingDialogListener()
          {
            @Override
            public void onDownload()
            {
              closeRouting();
              ActiveCountryTree.downloadMapsForIndex(missingCountries, StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING);
              showDownloader(true);
            }

            @Override
            public void onCancel()
            {
              closeRouting();
            }

            @Override
            public void onOk()
            {
              closeRouting();
              if (RoutingResultCodesProcessor.isDownloadable(resultCode))
                showDownloader(false);
            }
          });
          fragment.show(getSupportFragmentManager(), RoutingErrorDialogFragment.class.getName());
        }

        refreshZoomButtonsVisibility();
      }
    });
  }

  @Override
  public void customOnNavigateUp()
  {
    if (popFragment())
    {
      InputUtils.hideKeyboard(mBottomButtons);
      mFadeView.fadeOut(false);
      refreshRouterIcon();
    }
  }

  public interface MapTask extends Serializable
  {
    public boolean run(MWMActivity target);
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
    public boolean run(MWMActivity target)
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
    public boolean run(MWMActivity target)
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
    public boolean run(final MWMActivity target)
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
