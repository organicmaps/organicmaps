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
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.content.LocalBroadcastManager;
import android.telephony.TelephonyManager;
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
import com.mapswithme.maps.Ads.AdsManager;
import com.mapswithme.maps.Ads.Banner;
import com.mapswithme.maps.Ads.BannerDialogFragment;
import com.mapswithme.maps.Ads.MenuAd;
import com.mapswithme.maps.Framework.OnBalloonListener;
import com.mapswithme.maps.Framework.RoutingListener;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.background.WorkerService;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.ApiPoint;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.location.LocationPredictor;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.maps.search.SearchController;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.settings.StoragePathManager.SetStoragePathListener;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.widget.MapInfoView;
import com.mapswithme.maps.widget.MapInfoView.OnVisibilityChangedListener;
import com.mapswithme.maps.widget.MapInfoView.State;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Constants;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.Statistics;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.view.ViewHelper;
import com.nvidia.devtech.NvEventQueueActivity;

import java.io.Serializable;
import java.util.List;
import java.util.Locale;
import java.util.Stack;

public class MWMActivity extends NvEventQueueActivity
    implements LocationService.LocationListener, OnBalloonListener, OnVisibilityChangedListener,
    OnClickListener, RoutingListener
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
  // Info box (place page).
  private MapInfoView mInfoView;
  private ImageView mIvStartRouting;
  private TextView mTvRoutingDistance;
  private RelativeLayout mRlRoutingBox;
  private RelativeLayout mLayoutRoutingGo;
  private ProgressBar mPbRoutingProgress;

  private SearchController mSearchController;
  private boolean mNeedCheckUpdate = true;
  private boolean mRenderingInitialized = false;
  private int mLocationStateModeListenerId = -1;
  // Initialized to invalid combination to force update on the first check
  private boolean mStorageAvailable = false;
  private boolean mStorageWritable = true;
  // toolbars
  private static final long VERT_TOOLBAR_ANIM_DURATION = 250;
  private ViewGroup mVerticalToolbar;
  private ViewGroup mToolbar;
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

  private LocationPredictor mLocationPredictor;

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

  public static Intent createUpdateMapsIntent()
  {
    return new Intent(MWMApplication.get(), DownloadResourcesActivity.class)
        .putExtra(DownloadResourcesActivity.EXTRA_UPDATE_COUNTRIES, true);
  }

  private void pauseLocation()
  {
    LocationService.INSTANCE.stopUpdate(this);
    // Enable automatic turning screen off while app is idle
    Utils.automaticIdleScreen(true, getWindow());
  }

  private void listenLocationUpdates()
  {
    LocationService.INSTANCE.startUpdate(this);
    // Do not turn off the screen while displaying position
    Utils.automaticIdleScreen(false, getWindow());
  }

  /**
   * Invalidates location state in core(core then tries to restore previous location state and notifies us).
   * Updates location button accordingly.
   */
  public void invalidateLocationState()
  {
    final int currentLocationMode = LocationState.INSTANCE.getLocationStateMode();
    LocationButtonImageSetter.setButtonViewFromState(currentLocationMode, mLocationButton);
    LocationState.INSTANCE.invalidatePosition();
  }

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
          if (size > 50 * 1024 * 1024 && !ConnectionState.isWifiConnected())
          {
            new AlertDialog.Builder(MWMActivity.this)
                .setCancelable(true)
                .setMessage(String.format(getString(R.string.no_wifi_ask_cellular_download), MapStorage.INSTANCE.countryName(index)))
                .setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener()
                {
                  @Override
                  public void onClick(DialogInterface dlg, int which)
                  {
                    nativeDownloadCountry(index, options);
                    dlg.dismiss();
                  }
                })
                .setNegativeButton(getString(R.string.close), new DialogInterface.OnClickListener()
                {
                  @Override
                  public void onClick(DialogInterface dlg, int which)
                  {
                    dlg.dismiss();
                  }
                })
                .create()
                .show();

            return;
          }

          nativeDownloadCountry(index, options);
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
        mToolbar.getHandler().postDelayed(new Runnable()
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
        checkRoutingMaps();
        checkLiteMapsInPro();
        checkFacebookDialog();
        checkBuyProDialog();
        checkUserMarkActivation();
        checkShouldShowBanners();
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
    if (!MWMApplication.get().hasBookmarks())
      UiUtils.showBuyProDialog(this, getString(R.string.bookmarks_in_pro_version));
    else
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

  private void checkRoutingMaps()
  {
    if (BuildConfig.IS_PRO && MWMApplication.get().nativeGetBoolean(IS_FIRST_ROUTING_VERSION_RUN, true)
        && ActiveCountryTree.getOutOfDateCount() != 0)
    {
      MWMApplication.get().nativeSetBoolean(IS_FIRST_ROUTING_VERSION_RUN, false);
      new AlertDialog.Builder(this)
          .setCancelable(false)
          .setMessage(getString(R.string.routing_update_maps))
          .setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener()
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

  /**
   * Checks if PRO version is running on KITKAT or greater sdk.
   * If so - checks whether LITE version is installed and contains maps on sd card and then copies them to own directory on sdcard.
   */

  private void checkLiteMapsInPro()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT &&
        BuildConfig.IS_PRO &&
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
          runDownloadActivity(false);
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
    alignControls();
  }

  private void showDialogImpl(final int dlgID, int resMsg, DialogInterface.OnClickListener okListener)
  {
    new AlertDialog.Builder(this)
        .setCancelable(false)
        .setMessage(getString(resMsg))
        .setPositiveButton(getString(R.string.ok), okListener)
        .setNeutralButton(getString(R.string.never), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();
            MWMApplication.get().submitDialogResult(dlgID, MWMApplication.NEVER);
          }
        })
        .setNegativeButton(getString(R.string.later), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();
            MWMApplication.get().submitDialogResult(dlgID, MWMApplication.LATER);
          }
        })
        .create()
        .show();
  }

  private boolean isChinaISO(String iso)
  {
    final String arr[] = {"CN", "CHN", "HK", "HKG", "MO", "MAC"};
    for (final String s : arr)
      if (iso.equalsIgnoreCase(s))
        return true;
    return false;
  }

  private boolean isChinaRegion()
  {
    final TelephonyManager tm = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
    if (tm != null && tm.getPhoneType() != TelephonyManager.PHONE_TYPE_CDMA)
    {
      final String iso = tm.getNetworkCountryIso();
      Log.i(TAG, "TelephonyManager country ISO = " + iso);
      if (isChinaISO(iso))
        return true;
    }
    else
    {
      final Location l = LocationService.INSTANCE.getLastLocation();
      if (l != null && nativeIsInChina(l.getLatitude(), l.getLongitude()))
        return true;
      else
      {
        final String code = Locale.getDefault().getCountry();
        Log.i(TAG, "Locale country ISO = " + code);
        if (isChinaISO(code))
          return true;
      }
    }

    return false;
  }

  private void checkFacebookDialog()
  {
    if (ConnectionState.isConnected() &&
        MWMApplication.get().shouldShowDialog(MWMApplication.FACEBOOK) &&
        !isChinaRegion())
    {
      showDialogImpl(MWMApplication.FACEBOOK, R.string.share_on_facebook_text,
          new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              MWMApplication.get().submitDialogResult(MWMApplication.FACEBOOK, MWMApplication.OK);

              dlg.dismiss();
              UiUtils.showFacebookPage(MWMActivity.this);
            }
          }
      );
    }
  }

  private void checkBuyProDialog()
  {
    if (!BuildConfig.IS_PRO &&
        (ConnectionState.isConnected()) &&
        MWMApplication.get().shouldShowDialog(MWMApplication.BUYPRO))
    {
      showDialogImpl(MWMApplication.BUYPRO, R.string.pro_version_available,
          new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              MWMApplication.get().submitDialogResult(MWMApplication.BUYPRO, MWMApplication.OK);
              dlg.dismiss();
              UiUtils.openAppInMarket(MWMActivity.this, BuildConfig.PRO_URL);
            }
          }
      );
    }
  }

  private void runSearchActivity()
  {
    startActivity(new Intent(this, SearchActivity.class));
  }

  public void onSearchClicked(View v)
  {
    if (!BuildConfig.IS_PRO)
      UiUtils.showBuyProDialog(this, getString(R.string.search_available_in_pro_version));
    else if (!MapStorage.INSTANCE.updateMaps(R.string.search_update_maps, this, new MapStorage.UpdateFunctor()
    {
      @Override
      public void doUpdate()
      {
        runDownloadActivity(false);
      }

      @Override
      public void doCancel()
      {
        runSearchActivity();
      }
    }))
    {
      runSearchActivity();
    }
  }

  @Override
  public boolean onSearchRequested()
  {
    onSearchClicked(null);
    return false;
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
          mVerticalToolbar.setVisibility(View.VISIBLE);
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
          mVerticalToolbar.setVisibility(View.INVISIBLE);
          mFadeView.setVisibility(View.GONE);
          mVerticalToolbarAnimation = null;
        }
      };
    }

    // slide vertical toolbar
    mVerticalToolbarAnimation = UiUtils.generateRelativeSlideAnimation(0, 0, fromY, toY);
    mVerticalToolbarAnimation.setDuration(VERT_TOOLBAR_ANIM_DURATION);
    mVerticalToolbarAnimation.setAnimationListener(listener);
    mVerticalToolbar.startAnimation(mVerticalToolbarAnimation);

    // fade map
    Animation alphaAnimation = new AlphaAnimation(fromAlpha, toAlpha);
    alphaAnimation.setFillBefore(true);
    alphaAnimation.setFillAfter(true);
    alphaAnimation.setDuration(VERT_TOOLBAR_ANIM_DURATION);
    mFadeView.setVisibility(View.VISIBLE);
    mFadeView.startAnimation(alphaAnimation);
  }

  private void shareMyLocation()
  {
    final Location loc = LocationService.INSTANCE.getLastLocation();
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

  private void runDownloadActivity(boolean openDownloadedList)
  {
    final Intent intent = new Intent(this, DownloadActivity.class).putExtra(DownloadActivity.EXTRA_OPEN_DOWNLOADED_LIST, openDownloadedList);
    startActivity(intent);
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
    MWMApplication.get().onMwmStart(this);

    // Do not turn off the screen while benchmarking
    if (MWMApplication.get().nativeIsBenchmarking())
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    nativeConnectDownloadButton();

    // Set up view
    mLocationButton = (ImageButton) findViewById(R.id.map_button_myposition);

    yotaSetup();

    setUpInfoBox();
    setUpRoutingBox();

    Framework.nativeSetRoutingListener(this);
    Framework.nativeConnectBalloonListeners(this);

    final Intent intent = getIntent();
    // We need check for tasks both in onCreate and onNewIntent
    // because of bug in OS: https://code.google.com/p/android/issues/detail?id=38629
    addTask(intent);

    mSearchController = SearchController.getInstance();
    mSearchController.onCreate(this);

    setUpToolbars();

    if (intent != null && intent.hasExtra(EXTRA_SCREENSHOTS_TASK))
    {
      String value = intent.getStringExtra(EXTRA_SCREENSHOTS_TASK);
      if (value.equals(SCREENSHOTS_TASK_LOCATE))
        onMyPositionClicked(null);
    }

    updateToolbarAds();
    LocalBroadcastManager.getInstance(this).registerReceiver(mUpdateAdsReceiver, new IntentFilter(WorkerService.ACTION_UPDATE_MENU_ADS));

    mLocationPredictor = new LocationPredictor(new Handler(), this);
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
    mToolbar = (ViewGroup) findViewById(R.id.map_bottom_toolbar);
    mVerticalToolbar = (ViewGroup) findViewById(R.id.map_bottom_vertical_toolbar);
    mVerticalToolbar.findViewById(R.id.btn_buy_pro).setOnClickListener(this);
    mVerticalToolbar.findViewById(R.id.btn_download_maps).setOnClickListener(this);
    mVerticalToolbar.findViewById(R.id.btn_share).setOnClickListener(this);
    mVerticalToolbar.findViewById(R.id.btn_settings).setOnClickListener(this);
    View moreApps = mVerticalToolbar.findViewById(R.id.btn_more_apps);
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB || Framework.getGuideIds().length == 0)
      UiUtils.hide(moreApps);
    else
      moreApps.setOnClickListener(this);

    UiUtils.invisible(mVerticalToolbar);

    mFadeView = findViewById(R.id.fade_view);
  }

  private void setUpInfoBox()
  {
    mInfoView = (MapInfoView) findViewById(R.id.info_box);
    mInfoView.setOnVisibilityChangedListener(this);
    mInfoView.bringToFront();
    mIvStartRouting = (ImageView) mInfoView.findViewById(R.id.iv__start_routing);
    mIvStartRouting.setOnClickListener(this);
    mPbRoutingProgress = (ProgressBar) mInfoView.findViewById(R.id.pb__routing_progress);
  }

  private void setUpRoutingBox()
  {
    mRlRoutingBox = (RelativeLayout) findViewById(R.id.rl__routing_box);
    mRlRoutingBox.setVisibility(View.GONE);
    mRlRoutingBox.findViewById(R.id.iv__routing_close).setOnClickListener(this);
    mLayoutRoutingGo = (RelativeLayout) mRlRoutingBox.findViewById(R.id.rl__routing_go);
    mLayoutRoutingGo.setOnClickListener(this);
    mTvRoutingDistance = (TextView) mRlRoutingBox.findViewById(R.id.tv__routing_distance);
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

    if (intent != null)
    {
      if (intent.hasExtra(EXTRA_TASK))
        addTask(intent);
      else if (intent.hasExtra(EXTRA_SEARCH_RES_SINGLE))
      {
        final boolean singleResult = intent.getBooleanExtra(EXTRA_SEARCH_RES_SINGLE, false);
        if (singleResult)
        {
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

  private void alignControls()
  {
  }

  /// @name From Location interface
  //@{
  @Override
  public void onLocationError(int errorCode)
  {
    nativeOnLocationError(errorCode);

    // Notify user about turned off location services
    if (errorCode == LocationService.ERROR_DENIED)
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
    else if (errorCode == LocationService.ERROR_GPS_OFF)
    {
      Toast.makeText(this, R.string.gps_is_disabled_long_text, Toast.LENGTH_LONG).show();
    }
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    if (l.getProvider() != LocationService.LOCATION_PREDICTOR_PROVIDER)
      mLocationPredictor.reset(l);

    nativeLocationUpdated(
        l.getTime(),
        l.getLatitude(),
        l.getLongitude(),
        l.getAccuracy(),
        l.getAltitude(),
        l.getSpeed(),
        l.getBearing());

    if (mInfoView.getState() != State.HIDDEN)
      mInfoView.updateLocation(l);

    updateRoutingDistance();
  }

  private void updateRoutingDistance()
  {
    final LocationState.RoutingInfo info = Framework.nativeGetRouteFollowingInfo();
    if (info != null)
    {
      final SpannableStringBuilder builder = new SpannableStringBuilder(info.mDistToTarget).append(" ").append(info.mUnits.toUpperCase());
      builder.setSpan(new AbsoluteSizeSpan(34, true), 0, info.mDistToTarget.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      builder.setSpan(new AbsoluteSizeSpan(10, true), info.mDistToTarget.length(), builder.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      mTvRoutingDistance.setText(builder);
    }
  }

  @SuppressWarnings("deprecation")
  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    final double angles[] = {magneticNorth, trueNorth};
    LocationUtils.correctCompassAngles(getWindowManager().getDefaultDisplay().getOrientation(), angles);
    nativeCompassUpdated(time, angles[0], angles[1], accuracy);

    final double north = (angles[1] >= 0.0) ? angles[1] : angles[0];
    if (mInfoView.getState() != State.HIDDEN)
      mInfoView.updateAzimuth(north);
  }

  /// Callback from native location GUI element processing.
  public void onLocationStateModeChangedCallback(final int newMode)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
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
    });
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
  protected void onPause()
  {
    pauseLocation();

    stopWatchingExternalStorage();

    stopWatchingCompassStatusUpdate();

    super.onPause();
    mLocationPredictor.pause();
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

    alignControls();

    mSearchController.onResume();
    mInfoView.onResume();
    tryResumeRouting();

    MWMApplication.get().onMwmResume(this);
    mLocationPredictor.resume();
  }

  private void checkShouldShowBanners()
  {
    final Banner banner = AdsManager.getBannerToShow();
    if (banner != null)
    {
      final DialogFragment fragment = new BannerDialogFragment();
      fragment.setStyle(DialogFragment.STYLE_NORMAL, R.style.MWMTheme_Dialog_DialogFragment);
      final Bundle args = new Bundle();
      args.putParcelable(BannerDialogFragment.EXTRA_BANNER, banner);
      fragment.setArguments(args);
      final FragmentManager fragmentManager = getSupportFragmentManager();
      FragmentTransaction transaction = fragmentManager.beginTransaction();
      transaction.setTransition(FragmentTransaction.TRANSIT_FRAGMENT_OPEN);
      transaction.add(android.R.id.content, fragment).addToBackStack(null).commit();
    }
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
    if (mInfoView.getState() != State.HIDDEN)
    {
      hideInfoView();
      Framework.deactivatePopup();
    }
    else if (mVerticalToolbar.getVisibility() == View.VISIBLE)
      setVerticalToolbarVisible(false);
    else
      super.onBackPressed();
  }

  /// Callbacks from native map objects touch event.

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
          mInfoView.bringToFront();
          final String poiType = ParsedMmwRequest.getCurrentRequest().getCallerName(MWMApplication.get()).toString();
          final ApiPoint apiPoint = new ApiPoint(name, id, poiType, lat, lon);

          if (!mInfoView.hasMapObject(apiPoint))
          {
            mInfoView.setMapObject(apiPoint);
            mInfoView.setState(State.PREVIEW_ONLY);
            mIvStartRouting.setVisibility(View.VISIBLE);
            mPbRoutingProgress.setVisibility(View.GONE);
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
        mInfoView.bringToFront();
        if (!mInfoView.hasMapObject(poi))
        {
          mInfoView.setMapObject(poi);
          mInfoView.setState(State.PREVIEW_ONLY);
          mIvStartRouting.setVisibility(View.VISIBLE);
          mPbRoutingProgress.setVisibility(View.GONE);
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
        mInfoView.bringToFront();
        final Bookmark b = BookmarkManager.getBookmarkManager().getBookmark(category, bookmarkIndex);
        if (!mInfoView.hasMapObject(b))
        {
          mInfoView.setMapObject(b);
          mInfoView.setState(State.PREVIEW_ONLY);
          mIvStartRouting.setVisibility(View.VISIBLE);
          mPbRoutingProgress.setVisibility(View.GONE);
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
          mInfoView.bringToFront();
          if (!mInfoView.hasMapObject(mypos))
          {
            mInfoView.setMapObject(mypos);
            mInfoView.setState(State.PREVIEW_ONLY);
            mIvStartRouting.setVisibility(View.GONE);
            mPbRoutingProgress.setVisibility(View.GONE);
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
        mInfoView.bringToFront();
        final MapObject sr = new MapObject.SearchResult(name, type, lat, lon);
        if (!mInfoView.hasMapObject(sr))
        {
          mInfoView.setMapObject(sr);
          mInfoView.setState(State.PREVIEW_ONLY);
          mPbRoutingProgress.setVisibility(View.GONE);
          mIvStartRouting.setVisibility(View.VISIBLE);
        }
      }
    });
  }

  private void hideInfoView()
  {
    mInfoView.setState(State.HIDDEN);
    mInfoView.setMapObject(null);

    UiUtils.show(findViewById(R.id.map_bottom_toolbar));
  }

  @Override
  public void onDismiss()
  {
    if (!mInfoView.hasMapObject(null))
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

    case R.id.btn_buy_pro:
      setVerticalToolbarVisible(false);
      UiUtils.openAppInMarket(MWMActivity.this, BuildConfig.PRO_URL);
      break;
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
      runDownloadActivity(false);
      break;
    case R.id.btn_more_apps:
      setVerticalToolbarVisible(false);
      startActivity(new Intent(this, MoreAppsActivity.class));
      break;
    case R.id.iv__start_routing:
      buildRoute();
      break;
    case R.id.iv__routing_close:
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

    Animator animator = ObjectAnimator.ofFloat(mLayoutRoutingGo, "alpha", 1, 0);
    animator.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        mLayoutRoutingGo.setVisibility(View.GONE);
      }
    });
    animator.start();
  }

  private void buildRoute()
  {
    if (!BuildConfig.IS_PRO)
    {
      UiUtils.showBuyProDialog(this, getString(R.string.routing_failed_buy_pro));
      return;
    }
    if (!MWMApplication.get().nativeGetBoolean(IS_ROUTING_DISCLAIMER_APPROVED, false))
    {
      showRoutingDisclaimer();
      return;
    }

    mIvStartRouting.setVisibility(View.INVISIBLE);
    mPbRoutingProgress.setVisibility(View.VISIBLE);
    Framework.nativeBuildRoute(mInfoView.getMapObject().getLat(), mInfoView.getMapObject().getLon());
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
    mInfoView.bringToFront();
    mRlRoutingBox.setVisibility(View.GONE);
    mRlRoutingBox.clearAnimation();
    mPbRoutingProgress.setVisibility(View.GONE);
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
    if (mInfoView.getState() == State.FULL_PLACEPAGE)
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
          ViewHelper.setAlpha(mLayoutRoutingGo, 1);
          mLayoutRoutingGo.setVisibility(View.VISIBLE);

          Animation alphaAnimation = new AlphaAnimation(0.0f, 1.0f);
          alphaAnimation.setFillBefore(true);
          alphaAnimation.setFillAfter(true);
          alphaAnimation.setDuration(VERT_TOOLBAR_ANIM_DURATION);
          mRlRoutingBox.startAnimation(alphaAnimation);
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
                    // TODO start country download automatically?
                    runDownloadActivity(true);
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
    public boolean run(MWMActivity target)
    {
      target.runDownloadActivity(true);
      return true;
    }
  }
}
