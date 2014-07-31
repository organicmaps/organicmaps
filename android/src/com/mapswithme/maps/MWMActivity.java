package com.mapswithme.maps;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Point;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.mapswithme.country.DownloadActivity;
import com.mapswithme.maps.Framework.OnBalloonListener;
import com.mapswithme.maps.LocationButtonImageSetter.ButtonState;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.BookmarkActivity;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.ApiPoint;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.maps.promo.ActivationSettings;
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
import com.nvidia.devtech.NvEventQueueActivity;

import java.io.Serializable;
import java.util.Locale;
import java.util.Stack;

public class MWMActivity extends NvEventQueueActivity
    implements LocationService.LocationListener,
    OnBalloonListener,
    OnVisibilityChangedListener, OnClickListener
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
  private final Stack<MapTask> mTasks = new Stack<MapTask>();
  private MWMApplication mApplication = null;
  private BroadcastReceiver mExternalStorageReceiver = null;
  private StoragePathManager mPathManager = new StoragePathManager();
  private AlertDialog mStorageDisconnectedDialog = null;
  private ImageButton mLocationButton;
  // Info box (place page).
  private MapInfoView mInfoView;
  private SearchController mSearchController;
  private boolean mNeedCheckUpdate = true;
  private boolean mRenderingInitialized = false;
  private int mCompassStatusListenerID = -1;
  // Initialized to invalid combination to force update on the first check
  private boolean mStorageAvailable = false;
  private boolean mStorageWritable = true;
  private ViewGroup mVerticalToolbar;
  private ViewGroup mToolbar;

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

  private native void deactivatePopup();

  private LocationService getLocationService()
  {
    return mApplication.getLocationService();
  }

  private MapStorage getMapStorage()
  {
    return mApplication.getMapStorage();
  }

  private LocationState getLocationState()
  {
    return mApplication.getLocationState();
  }

  private void startLocation()
  {
    getLocationState().onStartLocation();
    resumeLocation();
  }

  private void stopLocation()
  {
    getLocationState().onStopLocation();
    pauseLocation();
  }

  private void pauseLocation()
  {
    getLocationService().stopUpdate(this);
    // Enable automatic turning screen off while app is idle
    Utils.automaticIdleScreen(true, getWindow());
  }

  private void resumeLocation()
  {
    getLocationService().startUpdate(this);
    // Do not turn off the screen while displaying position
    Utils.automaticIdleScreen(false, getWindow());
  }

  public void checkShouldResumeLocationService()
  {
    final LocationState state = getLocationState();
    final boolean hasPosition = state.hasPosition();
    final boolean isFollowMode = (state.getCompassProcessMode() == LocationState.COMPASS_FOLLOW);

    if (hasPosition || state.isFirstPosition())
    {
      if (hasPosition && isFollowMode)
      {
        state.startCompassFollowing();
        LocationButtonImageSetter.setButtonViewFromState(ButtonState.FOLLOW_MODE, mLocationButton);
      }
      else
      {
        LocationButtonImageSetter.setButtonViewFromState(
            hasPosition
                ? ButtonState.HAS_LOCATION
                : ButtonState.WAITING_LOCATION, mLocationButton
        );
      }
      resumeLocation();
    }
    else
    {
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.NO_LOCATION, mLocationButton);
    }
  }

  public void OnDownloadCountryClicked()
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        nativeDownloadCountry();
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
        checkFacebookDialog();
        checkBuyProDialog();
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
    if (!mApplication.hasBookmarks())
      showProVersionBanner(getString(R.string.bookmarks_in_pro_version));
    else
      startActivity(new Intent(this, BookmarkCategoriesActivity.class));
  }

  public void onMyPositionClicked(View v)
  {
    final LocationState state = mApplication.getLocationState();
    if (state.hasPosition())
    {
      if (state.isCentered())
      {
        if (mApplication.isProVersion() && state.hasCompass())
        {
          final boolean isFollowMode = (state.getCompassProcessMode() == LocationState.COMPASS_FOLLOW);
          if (isFollowMode)
          {
            state.stopCompassFollowingAndRotateMap();
          }
          else
          {
            state.startCompassFollowing();
            LocationButtonImageSetter.setButtonViewFromState(ButtonState.FOLLOW_MODE, mLocationButton);
            return;
          }
        }
      }
      else
      {
        state.animateToPositionAndEnqueueLocationProcessMode(LocationState.LOCATION_CENTER_ONLY);
        mLocationButton.setSelected(true);
        return;
      }
    }
    else
    {
      if (!state.isFirstPosition())
      {
        LocationButtonImageSetter.setButtonViewFromState(ButtonState.WAITING_LOCATION, mLocationButton);
        startLocation();
        return;
      }
    }

    // Stop location observing first ...
    stopLocation();
    LocationButtonImageSetter.setButtonViewFromState(ButtonState.NO_LOCATION, mLocationButton);
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
    final String KmlMovedFlag = "KmlBeenMoved";
    final String KitKatMigrationCompleted = "KitKatMigrationCompleted";
    final boolean kmlMoved = MWMApplication.get().nativeGetBoolean(KmlMovedFlag, false);
    final boolean mapsCpy = MWMApplication.get().nativeGetBoolean(KitKatMigrationCompleted, false);

    if (!kmlMoved)
    {
      if (mPathManager.moveBookmarks())
        mApplication.nativeSetBoolean(KmlMovedFlag, true);
      else
      {
        ShowAlertDlg(R.string.bookmark_move_fail);
        return;
      }
    }

    if (!mapsCpy)
    {
      SetStoragePathListener listener = new SetStoragePathListener()
      {
        @Override
        public void moveFilesFinished(String newPath)
        {
          mApplication.nativeSetBoolean(KitKatMigrationCompleted, true);
          ShowAlertDlg(R.string.kitkat_migrate_ok);
        }

        @Override
        public void moveFilesFailed()
        {
          ShowAlertDlg(R.string.kitkat_migrate_failed);
        }
      };
      mPathManager.checkWritableDir(this, listener);
    }
  }

  private void checkUpdateMaps()
  {
    // do it only once
    if (mNeedCheckUpdate)
    {
      mNeedCheckUpdate = false;

      getMapStorage().updateMaps(R.string.advise_update_maps, this, new MapStorage.UpdateFunctor()
      {
        @Override
        public void doUpdate()
        {
          runDownloadActivity();
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
            mApplication.submitDialogResult(dlgID, MWMApplication.NEVER);
          }
        })
        .setNegativeButton(getString(R.string.later), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            dlg.dismiss();
            mApplication.submitDialogResult(dlgID, MWMApplication.LATER);
          }
        })
        .create()
        .show();
  }

  private void showFacebookPage()
  {
    try
    {
      // Exception is thrown if we don't have installed Facebook application.
      getPackageManager().getPackageInfo(Constants.Package.FB_PACKAGE, 0);

      startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_MAPSME_COMMUNITY_NATIVE)));
    } catch (final Exception e)
    {
      startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_MAPSME_COMMUNITY_HTTP)));
    }
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
      final Location l = mApplication.getLocationService().getLastKnown();
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
    if (ConnectionState.isConnected(this) &&
        mApplication.shouldShowDialog(MWMApplication.FACEBOOK) &&
        !isChinaRegion())
    {
      showDialogImpl(MWMApplication.FACEBOOK, R.string.share_on_facebook_text,
          new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              mApplication.submitDialogResult(MWMApplication.FACEBOOK, MWMApplication.OK);

              dlg.dismiss();
              showFacebookPage();
            }
          }
      );
    }
  }

  private void showProVersionBanner(final String message)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        UiUtils.showBuyProDialog(MWMActivity.this, message);
      }
    });
  }

  private void checkBuyProDialog()
  {
    if (!mApplication.isProVersion() &&
        (ConnectionState.isConnected(this)) &&
        mApplication.shouldShowDialog(MWMApplication.BUYPRO))
    {
      showDialogImpl(MWMApplication.BUYPRO, R.string.pro_version_available,
          new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              mApplication.submitDialogResult(MWMApplication.BUYPRO, MWMApplication.OK);
              dlg.dismiss();
              UiUtils.runProMarketActivity(MWMActivity.this);
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
    if (!(mApplication.isProVersion() || ActivationSettings.isSearchActivated(this)))
    {
      showProVersionBanner(getString(R.string.search_available_in_pro_version));
    }
    else if (!getMapStorage().updateMaps(R.string.search_update_maps, this, new MapStorage.UpdateFunctor()
    {
      @Override
      public void doUpdate()
      {
        runDownloadActivity();
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
    UiUtils.show(mVerticalToolbar);
    UiUtils.hide(mToolbar);
  }

  private void shareMyLocation()
  {
    final Location loc = MWMApplication.get().getLocationService().getLastKnown();
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

  private void runDownloadActivity()
  {
    startActivity(new Intent(this, DownloadActivity.class));
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
    mApplication = (MWMApplication) getApplication();

    // Log app start events - successful installation means that user has passed DownloadResourcesActivity
    mApplication.onMwmStart(this);

    // Do not turn off the screen while benchmarking
    if (mApplication.nativeIsBenchmarking())
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    nativeConnectDownloadButton();

    // Set up view
    mLocationButton = (ImageButton) findViewById(R.id.map_button_myposition);

    yotaSetup();

    setUpInfoBox();

    Framework.nativeConnectBalloonListeners(this);

    final Intent intent = getIntent();
    // We need check for tasks both in onCreate and onNewIntent
    // because of bug in OS: https://code.google.com/p/android/issues/detail?id=38629
    addTask(intent);

    // Initialize location service
    getLocationService();

    mSearchController = SearchController.getInstance();
    mSearchController.onCreate(this);

    setUpToolbars();

    if (intent != null && intent.hasExtra(EXTRA_SCREENSHOTS_TASK))
    {
      String value = intent.getStringExtra(EXTRA_SCREENSHOTS_TASK);
      if (value.equals(SCREENSHOTS_TASK_LOCATE))
        onMyPositionClicked(null);
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
    {
      UiUtils.hide(moreApps);
    }
    else
    {
      moreApps.setOnClickListener(this);
    }

    UiUtils.hide(mVerticalToolbar);
  }

  private void setUpInfoBox()
  {
    mInfoView = (MapInfoView) findViewById(R.id.info_box);
    mInfoView.setOnVisibilityChangedListener(this);
    mInfoView.bringToFront();
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

          final LocationState locState = mApplication.getLocationState();

          if (locState.hasPosition() && locState.isCentered())
            Yota.showLocation(getApplicationContext(), zoom);
          else
            Yota.showMap(getApplicationContext(), latLon[0], latLon[1], zoom, null, locState.hasPosition());

          Statistics.INSTANCE.trackBackscreenCall(getApplication(), "Map");
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
    final View zoomPlusButton = findViewById(R.id.map_button_plus);
    final RelativeLayout.LayoutParams lp = (RelativeLayout.LayoutParams) zoomPlusButton.getLayoutParams();
    final int margin = (int) getResources().getDimension(R.dimen.zoom_margin);
    final int marginTop = (int) getResources().getDimension(R.dimen.zoom_plus_top_margin);
    lp.setMargins(margin, marginTop, margin, margin);
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
      getLocationState().turnOff();

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
    if (getLocationState().isFirstPosition())
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.HAS_LOCATION, mLocationButton);

    nativeLocationUpdated(l.getTime(), l.getLatitude(), l.getLongitude(), l.getAccuracy(), l.getAltitude(), l.getSpeed(), l.getBearing());
    if (mInfoView.getState() != State.HIDDEN)
      mInfoView.updateDistanceAndAzimut(l);
  }

  @SuppressWarnings("deprecation")
  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    final double angles[] = {magneticNorth, trueNorth};
    LocationUtils.correctCompassAngles(getWindowManager().getDefaultDisplay().getOrientation(), angles);
    nativeCompassUpdated(time, angles[0], angles[1], accuracy);
    final double north = (angles[1] >= 0.0 ? angles[1] : angles[0]);

    if (mInfoView.getState() != State.HIDDEN)
      mInfoView.updateAzimuth(north);
  }

  @Override
  public void onDrivingHeadingUpdated(long time, double heading, double accuracy)
  {
    LocationUtils.correctCompassAngles(getWindowManager().getDefaultDisplay().getOrientation(), new double[]{heading});
    nativeCompassUpdated(time, heading, heading, accuracy);

    if (mInfoView.getState() != State.HIDDEN)
      mInfoView.updateAzimuth(heading);
  }

  public void onCompassStatusChanged(int newStatus)
  {
    if (newStatus == 1)
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.FOLLOW_MODE, mLocationButton);
    else if (getLocationState().hasPosition())
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.HAS_LOCATION, mLocationButton);
    else
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.NO_LOCATION, mLocationButton);
  }

  /// Callback from native compass GUI element processing.
  public void OnCompassStatusChanged(int newStatus)
  {
    final int val = newStatus;
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        onCompassStatusChanged(val);
      }
    });
  }

  private void startWatchingCompassStatusUpdate()
  {
    mCompassStatusListenerID = mApplication.getLocationState().addCompassStatusListener(this);
  }

  private void stopWatchingCompassStatusUpdate()
  {
    mApplication.getLocationState().removeCompassStatusListener(mCompassStatusListenerID);
  }

  @Override
  protected void onPause()
  {
    pauseLocation();

    stopWatchingExternalStorage();

    stopWatchingCompassStatusUpdate();

    super.onPause();
  }

  @Override
  protected void onResume()
  {
    super.onResume();

    checkShouldResumeLocationService();

    startWatchingCompassStatusUpdate();

    startWatchingExternalStorage();

    UiUtils.showIf(mApplication.nativeGetBoolean(SettingsActivity.ZOOM_BUTTON_ENABLED, true),
        findViewById(R.id.map_button_plus),
        findViewById(R.id.map_button_minus));

    alignControls();

    mSearchController.onResume();
    mInfoView.onResume();

    getMwmApplication().onMwmResume(this);
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
      deactivatePopup();
    }
    else if (mVerticalToolbar.getVisibility() == View.VISIBLE)
    {
      UiUtils.show(mToolbar);
      UiUtils.hide(mVerticalToolbar);
    }
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
          final String poiType = ParsedMmwRequest.getCurrentRequest().getCallerName(mApplication).toString();
          final ApiPoint apiPoint = new ApiPoint(name, id, poiType, lat, lon);

          if (!mInfoView.hasMapObject(apiPoint))
          {
            mInfoView.setMapObject(apiPoint);
            mInfoView.setState(State.PREVIEW_ONLY);
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
        if (!mInfoView.hasMapObject(poi))
        {
          mInfoView.setMapObject(poi);
          mInfoView.setState(State.PREVIEW_ONLY);
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
        final Bookmark b = BookmarkManager.getBookmarkManager().getBookmark(category, bookmarkIndex);
        if (!mInfoView.hasMapObject(b))
        {
          mInfoView.setMapObject(b);
          mInfoView.setState(State.PREVIEW_ONLY);
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
        if (!mInfoView.hasMapObject(mypos))
        {
          mInfoView.setMapObject(mypos);
          mInfoView.setState(State.PREVIEW_ONLY);
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
        final MapObject sr = new MapObject.SearchResult(name, type, lat, lon);
        if (!mInfoView.hasMapObject(sr))
        {
          mInfoView.setMapObject(sr);
          mInfoView.setState(State.PREVIEW_ONLY);
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
          deactivatePopup();
        }
      });
    }
  }

  private native void nativeStorageConnected();

  private native void nativeStorageDisconnected();

  private native void nativeConnectDownloadButton();

  private native void nativeDownloadCountry();

  private native void nativeOnLocationError(int errorCode);

  private native void nativeLocationUpdated(long time, double lat, double lon, float accuracy, double altitude, float speed, float bearing);

  private native void nativeCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);

  private native boolean nativeIsInChina(double lat, double lon);

  public native boolean showMapForUrl(String url);

  @Override
  public void onPreviewVisibilityChanged(boolean isVisible)
  {
    UiUtils.hide(mVerticalToolbar);
  }

  @Override
  public void onPlacePageVisibilityChanged(boolean isVisible)
  {
    UiUtils.hide(mVerticalToolbar);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn_buy_pro:
      UiUtils.hide(mVerticalToolbar);
      UiUtils.show(mToolbar);
      UiUtils.runProMarketActivity(MWMActivity.this);
      break;
    case R.id.btn_share:
      UiUtils.hide(mVerticalToolbar);
      UiUtils.show(mToolbar);
      shareMyLocation();
      break;
    case R.id.btn_settings:
      UiUtils.hide(mVerticalToolbar);
      UiUtils.show(mToolbar);
      startActivity(new Intent(this, SettingsActivity.class));
      break;
    case R.id.btn_download_maps:
      UiUtils.hide(mVerticalToolbar);
      UiUtils.show(mToolbar);
      runDownloadActivity();
      break;
    case R.id.btn_more_apps:
      UiUtils.hide(mVerticalToolbar);
      UiUtils.show(mToolbar);
      startActivity(new Intent(this, MoreAppsActivity.class));
      break;
    default:
      break;
    }
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    UiUtils.hide(mVerticalToolbar);
    UiUtils.show(mToolbar);
    if (mInfoView.getState() == State.FULL_PLACEPAGE)
    {
      deactivatePopup();
      hideInfoView();
      return true;
    }
    return super.onTouch(view, event);
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
      final MapStorage storage = target.getMapStorage();
      if (mDoAutoDownload)
      {
        storage.downloadCountry(mIndex);
        // set zoom level so that download process is visible
        Framework.nativeShowCountry(mIndex, true);
      }
      else
        Framework.nativeShowCountry(mIndex, false);

      return true;
    }
  }

}
