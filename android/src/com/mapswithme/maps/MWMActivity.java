package com.mapswithme.maps;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.Point;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.widget.Toolbar;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
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
import com.mapswithme.maps.ads.AdsManager;
import com.mapswithme.maps.ads.LikesManager;
import com.mapswithme.maps.ads.MenuAd;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.background.WorkerService;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.ApiPoint;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.downloader.DownloadHelper;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPredictor;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchController;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.settings.StoragePathManager.SetStoragePathListener;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.widget.placepage.BasePlacePageAnimationController;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.Constants;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.log.MemLogging;
import com.mapswithme.util.statistics.Statistics;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.view.ViewHelper;
import com.nvidia.devtech.NvEventQueueActivity;

import java.io.Serializable;
import java.util.List;
import java.util.Stack;
import java.util.concurrent.TimeUnit;
import android.os.Debug;

public class MWMActivity extends NvEventQueueActivity
    implements LocationHelper.LocationListener, OnBalloonListener,
    BasePlacePageAnimationController.OnVisibilityChangedListener, OnClickListener, Framework.RoutingListener
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
  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();
  private BroadcastReceiver mExternalStorageReceiver = null;
  private StoragePathManager mPathManager = new StoragePathManager();
  private AlertDialog mStorageDisconnectedDialog = null;
  private ImageButton mLocationButton;
  // Place page
  private PlacePageView mPlacePage;
  private View mIvStartRouting;
  // Routing
  private TextView mTvRoutingDistance;
  private RelativeLayout mRlRoutingBox;
  private RelativeLayout mLayoutRoutingGo;
  private ProgressBar mPbRoutingProgress;
  private RelativeLayout mRlTurnByTurnBox;
  private TextView mTvTotalDistance;
  private TextView mTvTotalTime;
  private ImageView mIvTurn;
  private TextView mTvTurnDistance;

  private boolean mNeedCheckUpdate = true;
  private boolean mRenderingInitialized = false;
  private int mLocationStateModeListenerId = -1;
  // Initialized to invalid combination to force update on the first check
  private boolean mStorageAvailable = false;
  private boolean mStorageWritable = true;
  // toolbars
  private static final long VERT_TOOLBAR_ANIM_DURATION = 250;
  private ViewGroup mVerticalToolbar;
  private ViewGroup mBottomToolbar;
  private Animation mVerticalToolbarAnimation;
  private static final float FADE_VIEW_ALPHA = 0.5f;
  private View mFadeView;

  private static final String IS_KML_MOVED = "KmlBeenMoved";
  private static final String IS_KITKAT_MIGRATION_COMPLETED = "KitKatMigrationCompleted";
  // for routing
  private static final String IS_FIRST_ROUTING_VERSION_RUN = "IsFirstRoutingRun";
  private static final String IS_ROUTING_DISCLAIMER_APPROVED = "IsDisclaimerApproved";
  // ads in vertical toolbar
  private static final String MENU_ADS_ENABLED = "MenuLinksEnabled";
  private BroadcastReceiver mUpdateAdsReceiver = new BroadcastReceiver()
  {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      updateToolbarAds();
    }
  };
  private boolean mAreToolbarAdsUpdated;
  private boolean mIsFragmentContainer;

  private LocationPredictor mLocationPredictor;
  private LikesManager mLikesManager;

  private MemLogging mMemLogging;

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

  public static void startSearch(Context context, String query)
  {
    final MWMActivity activity = (MWMActivity) context;
    if (activity.mIsFragmentContainer)
      activity.showSearch();
    else
      SearchActivity.startForSearch(context, query);
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
    Utils.automaticIdleScreen(true, getWindow());
  }

  private void listenLocationUpdates()
  {
    LocationHelper.INSTANCE.addLocationListener(this);
    // Do not turn off the screen while displaying position
    Utils.automaticIdleScreen(false, getWindow());
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

  @SuppressWarnings("UnusedDeclaration")
  public void OnDownloadCountryClicked(final int group, final int country, final int region, final int options)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        final MapStorage.Index index = new Index(group, country, region);
        if (options == -1)
          nativeDownloadCountry(index, options);
        else
        {
          long size = MapStorage.INSTANCE.countryRemoteSizeInBytes(index, options);
          DownloadHelper.downloadWithCellularCheck(MWMActivity.this, size, MapStorage.INSTANCE.countryName(index), new DownloadHelper.OnDownloadListener()
          {
            @Override
            public void onDownload()
            {
              nativeDownloadCountry(index, options);
            }
          });
        }
      }
    });
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
        mBottomToolbar.getHandler().postDelayed(new Runnable()
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
  public void OnRenderingInitialized()
  {
    mRenderingInitialized = true;

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

  private Activity getActivity() { return this; }

  @Override
  public void ReportUnsupported()
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        new AlertDialog.Builder(getActivity())
            .setMessage(getString(R.string.unsupported_phone))
            .setCancelable(false)
            .setPositiveButton(getString(R.string.close), new DialogInterface.OnClickListener()
            {
              @Override
              public void onClick(DialogInterface dlg, int which)
              {
                getActivity().moveTaskToBack(true);
                dlg.dismiss();
              }
            })
            .create()
            .show();
      }
    });
  }

  private void checkMeasurementSystem()
  {
    UnitLocale.initializeCurrentUnits();
  }

  private native void nativeScale(double k);

  public void onPlusClicked(View v)
  {
    nativeScale(3.0 / 2);
  }

  public void onMinusClicked(View v)
  {
    nativeScale(2.0 / 3);
  }

  public void onBookmarksClicked(View v)
  {
    showBookmarks();
  }

  private void showBookmarks()
  {
    // TODO open in fragment?
    startActivity(new Intent(this, BookmarkCategoriesActivity.class));
  }

  public void onMyPositionClicked(View v)
  {
    final LocationState state = LocationState.INSTANCE;
    state.switchToNextMode();
  }

  private void ShowAlertDlg(int tittleID)
  {
    new AlertDialog.Builder(this)
        .setCancelable(false)
        .setMessage(tittleID)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which) { dlg.dismiss(); }
        })
        .create()
        .show();
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
        ShowAlertDlg(R.string.bookmark_move_fail);
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
              ShowAlertDlg(R.string.kitkat_migrate_ok);
            }

            @Override
            public void moveFilesFailed()
            {
              ShowAlertDlg(R.string.kitkat_migrate_failed);
            }
          }
      );
  }

  /**
   * Checks if PRO version is running on KITKAT or greater sdk.
   * If so - checks whether LITE version is installed and contains maps on sd card and then copies them to own directory on sdcard.
   */

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
              ShowAlertDlg(R.string.move_lite_maps_to_pro_ok);
            }

            @Override
            public void moveFilesFailed()
            {
              ShowAlertDlg(R.string.move_lite_maps_to_pro_failed);
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

  @Override
  public void onConfigurationChanged(Configuration newConfig)
  {
    super.onConfigurationChanged(newConfig);
  }

  private void showSearch()
  {
    if (mIsFragmentContainer)
    {
      if (getSupportFragmentManager().findFragmentByTag(SearchFragment.class.getName()) != null) // search is already shown
        return;
      setVerticalToolbarVisible(false);
      hideInfoView();
      Framework.deactivatePopup();
      popFragment();

      FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
      Fragment fragment = new SearchFragment();
      fragment.setArguments(getIntent().getExtras());
      transaction.setCustomAnimations(R.anim.abc_slide_in_bottom, R.anim.abc_slide_out_bottom,
          R.anim.abc_slide_in_bottom, R.anim.abc_slide_out_bottom);
      transaction.add(R.id.fragment_container, fragment, fragment.getClass().getName());
      transaction.addToBackStack(null).commit();
    }
    else
      startActivity(new Intent(this, SearchActivity.class));
  }

  public void onSearchClicked(View v)
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

  public void onMoreClicked(View v)
  {
    setVerticalToolbarVisible(true);
  }

  private void setVerticalToolbarVisible(boolean showVerticalToolbar)
  {
    if (mVerticalToolbarAnimation != null ||
        (mVerticalToolbar.getVisibility() == View.VISIBLE && showVerticalToolbar) ||
        (mVerticalToolbar.getVisibility() != View.VISIBLE && !showVerticalToolbar))
      return;

    hideInfoView();
    Framework.deactivatePopup();
    popFragment();

    int fromY, toY;
    Animation.AnimationListener listener;
    float fromAlpha, toAlpha;
    if (showVerticalToolbar)
    {
      fromY = 1;
      toY = 0;
      fromAlpha = 0.0f;
      toAlpha = FADE_VIEW_ALPHA;

      listener = new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationStart(Animation animation)
        {
          UiUtils.show(mVerticalToolbar);
        }

        @Override
        public void onAnimationEnd(Animation animation)
        {
          mVerticalToolbarAnimation = null;
        }
      };
    }
    else
    {
      fromY = 0;
      toY = 1;
      fromAlpha = FADE_VIEW_ALPHA;
      toAlpha = 0.0f;

      listener = new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.invisible(mVerticalToolbar);
          mVerticalToolbarAnimation = null;
        }
      };
    }

    mVerticalToolbarAnimation = UiUtils.generateRelativeSlideAnimation(0, 0, fromY, toY);
    mVerticalToolbarAnimation.setDuration(VERT_TOOLBAR_ANIM_DURATION);
    mVerticalToolbarAnimation.setAnimationListener(listener);
    mVerticalToolbar.startAnimation(mVerticalToolbarAnimation);

    // fade map
    fadeMap(fromAlpha, toAlpha);
  }

  private void fadeMap(float fromAlpha, final float toAlpha)
  {
    Animation alphaAnimation = new AlphaAnimation(fromAlpha, toAlpha);
    alphaAnimation.setFillBefore(true);
    alphaAnimation.setFillAfter(true);
    alphaAnimation.setDuration(VERT_TOOLBAR_ANIM_DURATION);
    alphaAnimation.setAnimationListener(new UiUtils.SimpleAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animation animation)
      {
        if (toAlpha == 0)
          mFadeView.setVisibility(View.GONE);
      }
    });
    mFadeView.setVisibility(View.VISIBLE);
    mFadeView.startAnimation(alphaAnimation);
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
      ShareAction.getAnyShare().shareWithText(getActivity(), body, "");
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
      setVerticalToolbarVisible(false);
      popFragment();
      hideInfoView();
      SearchController.getInstance().cancel();

      FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
      Fragment fragment = new DownloadFragment();
      final Bundle extras = new Bundle();
      extras.putBoolean(DownloadActivity.EXTRA_OPEN_DOWNLOADED_LIST, openDownloadedList);
      fragment.setArguments(extras);
      transaction.setCustomAnimations(R.anim.abc_slide_in_bottom, R.anim.abc_slide_out_bottom,
          R.anim.abc_slide_in_bottom, R.anim.abc_slide_out_bottom);
      transaction.add(R.id.fragment_container, fragment, fragment.getClass().getName());
      transaction.addToBackStack(null).commit();

      fadeMap(0, FADE_VIEW_ALPHA);
    }
    else
    {
      final Intent intent = new Intent(this, DownloadActivity.class).putExtra(DownloadActivity.EXTRA_OPEN_DOWNLOADED_LIST, openDownloadedList);
      startActivity(intent);
    }
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    // Use full-screen on Kindle Fire only
    if (Utils.isAmazonDevice())
    {
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
      getWindow().clearFlags(android.view.WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
    }
    setContentView(R.layout.map);
    super.onCreate(savedInstanceState);

    // Log app start events - successful installation means that user has passed DownloadResourcesActivity
    MWMApplication.get().onMwmCreate(this);

    // Do not turn off the screen while benchmarking
    if (MWMApplication.get().nativeIsBenchmarking())
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    nativeConnectDownloadButton();

    initViews();

    Framework.nativeSetRoutingListener(this);
    Framework.nativeConnectBalloonListeners(this);

    final Intent intent = getIntent();
    // We need check for tasks both in onCreate and onNewIntent
    // because of bug in OS: https://code.google.com/p/android/issues/detail?id=38629
    addTask(intent);

    SearchController.getInstance().onCreate(this);

    if (intent != null && intent.hasExtra(EXTRA_SCREENSHOTS_TASK))
    {
      String value = intent.getStringExtra(EXTRA_SCREENSHOTS_TASK);
      if (value.equals(SCREENSHOTS_TASK_LOCATE))
        onMyPositionClicked(null);
    }

    updateToolbarAds();
    LocalBroadcastManager.getInstance(this).registerReceiver(mUpdateAdsReceiver, new IntentFilter(WorkerService.ACTION_UPDATE_MENU_ADS));

    mLocationPredictor = new LocationPredictor(new Handler(), this);
    mLikesManager = new LikesManager(this);
    mMemLogging = new MemLogging(this);
  }

  private void initViews()
  {
    mLocationButton = (ImageButton) findViewById(R.id.map_button_myposition);
    yotaSetup();
    setUpPlacePage();
    setUpRoutingBox();
    setUpToolbars();
    if (findViewById(R.id.fragment_container) != null)
    {
      mIsFragmentContainer = true;
      // add dummy fragment to enable pop out fragment animations
      Fragment fragment = new Fragment();
      FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
      transaction.add(R.id.fragment_container, fragment);
      transaction.commit();
    }
  }

  private void updateToolbarAds()
  {
    final List<MenuAd> ads = AdsManager.getMenuAds();
    if (ads != null && !mAreToolbarAdsUpdated && MWMApplication.get().nativeGetBoolean(MENU_ADS_ENABLED, true))
    {
      mAreToolbarAdsUpdated = true;
      int startAdMenuPosition = 7;
      for (final MenuAd ad : ads)
      {
        final View view = getLayoutInflater().inflate(R.layout.item_bottom_toolbar, mVerticalToolbar, false);
        final TextView textView = (TextView) view.findViewById(R.id.tv__bottom_item_text);
        textView.setText(ad.getTitle());
        try
        {
          textView.setTextColor(Color.parseColor(ad.getHexColor()));
        } catch (IllegalArgumentException e)
        {
          e.printStackTrace();
        }
        final ImageView imageView = (ImageView) view.findViewById(R.id.iv__bottom_icon);
        imageView.setImageBitmap(ad.getIcon());

        view.setOnClickListener(new OnClickListener()
        {
          @Override
          public void onClick(View v)
          {
            final String appPackage = ad.getAppPackage();
            if (!TextUtils.isEmpty(appPackage) && Utils.isPackageInstalled(appPackage))
              Utils.launchPackage(MWMActivity.this, appPackage);
            else
            {
              final Intent it = new Intent(Intent.ACTION_VIEW);
              it.setData(Uri.parse(ad.getAppUrl()));
              startActivity(it);
            }
          }
        });
        mVerticalToolbar.addView(view, startAdMenuPosition++);
      }
    }
  }

  private void setUpToolbars()
  {
    mBottomToolbar = (ViewGroup) findViewById(R.id.map_bottom_toolbar);
    mVerticalToolbar = (ViewGroup) findViewById(R.id.map_bottom_vertical_toolbar);
    mVerticalToolbar.findViewById(R.id.btn_download_maps).setOnClickListener(this);
    mVerticalToolbar.findViewById(R.id.btn_share).setOnClickListener(this);
    mVerticalToolbar.findViewById(R.id.btn_settings).setOnClickListener(this);

    UiUtils.invisible(mVerticalToolbar);

    mFadeView = findViewById(R.id.fade_view);

    final Toolbar toolbar = (Toolbar) mVerticalToolbar.findViewById(R.id.toolbar);
    if (toolbar != null)
    {
      UiUtils.showHomeUpButton(toolbar);
      toolbar.setTitle(getString(R.string.toolbar_application_menu));
      toolbar.setNavigationOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          onBackPressed();
        }
      });
    }
  }

  private void setUpPlacePage()
  {
    mPlacePage = (PlacePageView) findViewById(R.id.info_box);
    mPlacePage.setOnVisibilityChangedListener(this);
    mIvStartRouting = mPlacePage.findViewById(R.id.rl__route);
    mIvStartRouting.setOnClickListener(this);
    mPbRoutingProgress = (ProgressBar) mPlacePage.findViewById(R.id.pb__routing_progress);
  }

  private void setUpRoutingBox()
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

  private void yotaSetup()
  {
    final View yopmeButton = findViewById(R.id.yop_it);
    if (!Yota.isYota())
    {
      yopmeButton.setVisibility(View.INVISIBLE);
    }
    else
    {
      yopmeButton.setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          final double[] latLon = Framework.getScreenRectCenter();
          final double zoom = Framework.getDrawScale();

          final LocationState locState = LocationState.INSTANCE;
          final int locationStateMode = locState.getLocationStateMode();

          if (locationStateMode > LocationState.NOT_FOLLOW)
            Yota.showLocation(getApplicationContext(), zoom);
          else
            Yota.showMap(getApplicationContext(), latLon[0], latLon[1], zoom, null, locationStateMode == LocationState.NOT_FOLLOW);

          Statistics.INSTANCE.trackBackscreenCall("Map");
        }
      });
    }
  }

  @Override
  public void onDestroy()
  {
    Framework.nativeClearBalloonListeners();

    super.onDestroy();
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);

    fadeMap(0, 0);
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
          onAdditionalLayerActivated(result.getName(), result.getPoiTypeName(), result.getLat(), result.getLon());
        }
        else
          onDismiss();
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

      if (mRenderingInitialized)
        runTasks();

      // mark intent as consumed
      intent.putExtra(EXTRA_CONSUMED, true);
    }
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    mRenderingInitialized = false;
  }

  @Override
  public void onLocationError(int errorCode)
  {
    nativeOnLocationError(errorCode);

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

    nativeLocationUpdated(
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

  @SuppressWarnings("deprecation")
  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    final double angles[] = {magneticNorth, trueNorth};
    LocationUtils.correctCompassAngles(getWindowManager().getDefaultDisplay().getOrientation(), angles);
    nativeCompassUpdated(time, angles[0], angles[1], accuracy);

    final double north = (angles[1] >= 0.0) ? angles[1] : angles[0];
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
    LocationButtonImageSetter.setButtonViewFromState(newMode, mLocationButton);
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

    UiUtils.showIf(MWMApplication.get().nativeGetBoolean(SettingsActivity.ZOOM_BUTTON_ENABLED, true),
        findViewById(R.id.map_button_plus),
        findViewById(R.id.map_button_minus));

    SearchController.getInstance().onResume();
    mPlacePage.onResume();
    tryResumeRouting();
    mLocationPredictor.resume();
    mLikesManager.showLikeDialogs();

    mMemLogging.startLogging();
  }

  @Override
  protected void onPause()
  {
    mMemLogging.stopLogging();

    pauseLocation();
    stopWatchingExternalStorage();
    stopWatchingCompassStatusUpdate();
    super.onPause();
    mLocationPredictor.pause();
    mLikesManager.cancelLikeDialogs();
  }

  private void tryResumeRouting()
  {
    if (Framework.nativeIsRoutingActive())
    {
      updateRoutingDistance();
      mRlRoutingBox.setVisibility(View.VISIBLE);
    }
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
      nativeStorageConnected();

      // @TODO enable downloader button and dismiss blocking popup

      if (mStorageDisconnectedDialog != null)
        mStorageDisconnectedDialog.dismiss();
    }
    else if (available)
    {
      // Add local maps to the model
      nativeStorageConnected();

      // @TODO disable downloader button and dismiss blocking popup

      if (mStorageDisconnectedDialog != null)
        mStorageDisconnectedDialog.dismiss();
    }
    else
    {
      // Remove local maps from the model
      nativeStorageDisconnected();

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
      hideInfoView();
      Framework.deactivatePopup();
    }
    else if (mVerticalToolbar.getVisibility() == View.VISIBLE)
      setVerticalToolbarVisible(false);
    else if (canFragmentInterceptBackPress())
      return;
    else if (popFragment())
    {
      InputUtils.hideKeyboard(mBottomToolbar);
      if (isMapFaded())
        fadeMap(FADE_VIEW_ALPHA, 0.0f);
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
    final int count = manager.getBackStackEntryCount();
    if (count < 1) // first fragment is dummy and shouldn't be removed
      return false;

    DownloadFragment fragment = (DownloadFragment) manager.findFragmentByTag(DownloadFragment.class.getName());
    return fragment != null && fragment.onBackPressed();
  }

  private boolean popFragment()
  {
    final FragmentManager manager = getSupportFragmentManager();
    final int count = manager.getBackStackEntryCount();
    if (count < 1) // first fragment is dummy and shouldn't be removed
      return false;

    InputUtils.hideKeyboard(mVerticalToolbar);
    Fragment fragment = manager.findFragmentByTag(SearchFragment.class.getName());
    if (fragment != null)
    {
      manager.popBackStack();
      return true;
    }
    fragment = manager.findFragmentByTag(DownloadFragment.class.getName());
    if (fragment != null)
    {
      manager.popBackStack();
      return true;
    }

    return false;
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
          mPlacePage.bringToFront();
          final String poiType = ParsedMmwRequest.getCurrentRequest().getCallerName(MWMApplication.get()).toString();
          final ApiPoint apiPoint = new ApiPoint(name, id, poiType, lat, lon);

          if (!mPlacePage.hasMapObject(apiPoint))
          {
            mPlacePage.setMapObject(apiPoint);
            mPlacePage.setState(State.PREVIEW_ONLY);
            mIvStartRouting.setVisibility(View.VISIBLE);
            mPbRoutingProgress.setVisibility(View.GONE);
            if (popFragment() && isMapFaded())
              fadeMap(FADE_VIEW_ALPHA, 0);
          }
        }
      });
    }
  }

  @Override
  public void onPoiActivated(final String name, final String type, final String address, final double lat, final double lon)
  {
    final MapObject poi = new MapObject.Poi(name, lat, lon, type);

    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        mPlacePage.bringToFront();
        if (!mPlacePage.hasMapObject(poi))
        {
          mPlacePage.setMapObject(poi);
          mPlacePage.setState(State.PREVIEW_ONLY);
          mIvStartRouting.setVisibility(View.VISIBLE);
          mPbRoutingProgress.setVisibility(View.GONE);
          if (popFragment() && isMapFaded())
            fadeMap(FADE_VIEW_ALPHA, 0);
        }
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
        mPlacePage.bringToFront();
        final Bookmark b = BookmarkManager.getBookmarkManager().getBookmark(category, bookmarkIndex);
        if (!mPlacePage.hasMapObject(b))
        {
          mPlacePage.setMapObject(b);
          mPlacePage.setState(State.PREVIEW_ONLY);
          mIvStartRouting.setVisibility(View.VISIBLE);
          mPbRoutingProgress.setVisibility(View.GONE);
          if (popFragment() && isMapFaded())
            fadeMap(FADE_VIEW_ALPHA, 0);
        }
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
          mPlacePage.bringToFront();
          if (!mPlacePage.hasMapObject(mypos))
          {
            mPlacePage.setMapObject(mypos);
            mPlacePage.setState(State.PREVIEW_ONLY);
            mIvStartRouting.setVisibility(View.GONE);
            mPbRoutingProgress.setVisibility(View.GONE);
            if (popFragment() && isMapFaded())
              fadeMap(FADE_VIEW_ALPHA, 0);
          }
        }
      }
    });
  }

  @Override
  public void onAdditionalLayerActivated(final String name, final String type, final double lat, final double lon)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        mPlacePage.bringToFront();
        final MapObject sr = new MapObject.SearchResult(name, type, lat, lon);
        if (!mPlacePage.hasMapObject(sr))
        {
          mPlacePage.setMapObject(sr);
          mPlacePage.setState(State.PREVIEW_ONLY);
          mPbRoutingProgress.setVisibility(View.GONE);
          mIvStartRouting.setVisibility(View.VISIBLE);
          if (popFragment() && isMapFaded())
            fadeMap(FADE_VIEW_ALPHA, 0);
        }
      }
    });
  }

  private void hideInfoView()
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
          hideInfoView();
          Framework.deactivatePopup();
        }
      });
    }
  }

  private native void nativeStorageConnected();

  private native void nativeStorageDisconnected();

  private native void nativeConnectDownloadButton();

  private native void nativeDownloadCountry(MapStorage.Index index, int options);

  private native void nativeOnLocationError(int errorCode);

  private native void nativeLocationUpdated(long time, double lat, double lon, float accuracy, double altitude, float speed, float bearing);

  private native void nativeCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);

  private native boolean nativeIsInChina(double lat, double lon);

  public native boolean showMapForUrl(String url);

  @Override
  public void onPreviewVisibilityChanged(boolean isVisible)
  {
    setVerticalToolbarVisible(false);
  }

  @Override
  public void onPlacePageVisibilityChanged(boolean isVisible)
  {
    setVerticalToolbarVisible(false);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {

    case R.id.btn_share:
      setVerticalToolbarVisible(false);
      shareMyLocation();
      break;
    case R.id.btn_settings:
      setVerticalToolbarVisible(false);
      startActivity(new Intent(this, SettingsActivity.class));
      break;
    case R.id.btn_download_maps:
      setVerticalToolbarVisible(false);
      showDownloader(false);
      break;
    case R.id.iv__start_routing:
      buildRoute();
      break;
    case R.id.iv__routing_close:
    case R.id.btn__close:
      closeRouting();
      break;
    case R.id.rl__routing_go:
      followRoute();
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
      mIvStartRouting.setVisibility(View.INVISIBLE);
      mPbRoutingProgress.setVisibility(View.VISIBLE);
      Framework.nativeBuildRoute(mapObject.getLat(), mapObject.getLon());
    }
    else
      Log.d(MWMActivity.class.getName(), "buildRoute(). MapObject is null. MapInfoView visibility : " + mPlacePage.getVisibility());
  }

  private void showRoutingDisclaimer()
  {
    new AlertDialog.Builder(getActivity())
        .setMessage(getString(R.string.routing_disclaimer))
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
    mRlRoutingBox.clearAnimation();
    UiUtils.hide(mRlRoutingBox, mPbRoutingProgress, mRlTurnByTurnBox);
    mIvStartRouting.setVisibility(View.VISIBLE);

    Framework.nativeCloseRouting();
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    boolean result = false;
    // if vertical toolbar is visible - hide it and ignore touch
    if (mVerticalToolbar.getVisibility() == View.VISIBLE)
    {
      setVerticalToolbarVisible(false);
      result = true;
    }
    if (mPlacePage.getState() == State.FULL_PLACEPAGE)
    {
      Framework.deactivatePopup();
      hideInfoView();
      result = true;
    }
    result |= super.onTouch(view, event);
    return result;
  }

  @Override
  public boolean onKeyUp(int keyCode, KeyEvent event)
  {
    if (keyCode == KeyEvent.KEYCODE_MENU)
    {
      setVerticalToolbarVisible(true);
      return true;
    }
    return super.onKeyUp(keyCode, event);
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == BookmarkActivity.REQUEST_CODE_EDIT_BOOKMARK && resultCode == RESULT_OK)
    {
      final Point bmk = ((ParcelablePoint) data.getParcelableExtra(BookmarkActivity.PIN)).getPoint();
      onBookmarkActivated(bmk.x, bmk.y);
    }

    super.onActivityResult(requestCode, resultCode, data);

  }

  @Override
  public void onRoutingEvent(final boolean isSuccess, final String message, final boolean openDownloader)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        if (isSuccess)
        {
          mRlTurnByTurnBox.setVisibility(View.GONE);
          ViewHelper.setAlpha(mLayoutRoutingGo, 1);
          mLayoutRoutingGo.setVisibility(View.VISIBLE);

          Animator animator = ObjectAnimator.ofFloat(mRlRoutingBox, "alpha", 0, 1);
          animator.setDuration(VERT_TOOLBAR_ANIM_DURATION);
          animator.start();

          mRlRoutingBox.setVisibility(View.VISIBLE);
          mRlRoutingBox.bringToFront();

          hideInfoView();
          Framework.deactivatePopup();
          updateRoutingDistance();
        }
        else
        {
          AlertDialog.Builder builder = new AlertDialog.Builder(MWMActivity.this)
              .setMessage(message)
              .setCancelable(true);
          if (openDownloader)
          {
            builder
                .setPositiveButton(android.R.string.ok, new Dialog.OnClickListener()
                {
                  @Override
                  public void onClick(DialogInterface dialog, int which)
                  {
                    final Location location = LocationHelper.INSTANCE.getLastLocation();
                    if (location != null)
                    {
                      final Index currentIndex = Framework.nativeGetCountryIndex(location.getLatitude(), location.getLongitude());
                      if (currentIndex != null)
                        ActiveCountryTree.downloadMapForIndex(currentIndex, StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING);
                    }
                    showDownloader(true);
                    closeRouting();
                    dialog.dismiss();
                  }
                })
                .setNegativeButton(android.R.string.cancel, new Dialog.OnClickListener()
                {
                  @Override
                  public void onClick(DialogInterface dialog, int which)
                  {
                    closeRouting();
                    dialog.dismiss();
                  }
                });
          }
          else
          {
            builder
                .setPositiveButton(android.R.string.ok, new Dialog.OnClickListener()
                {
                  @Override
                  public void onClick(DialogInterface dialog, int which)
                  {
                    closeRouting();
                    dialog.dismiss();
                  }
                });
          }
          builder.create().show();
        }
      }
    });
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
      return target.showMapForUrl(mUrl);
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
