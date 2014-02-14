package com.mapswithme.maps;

import java.io.Serializable;
import java.util.Locale;
import java.util.Stack;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v4.widget.DrawerLayout.DrawerListener;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.mapswithme.country.DownloadUI;
import com.mapswithme.maps.Framework.OnBalloonListener;
import com.mapswithme.maps.LocationButtonImageSetter.ButtonState;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.maps.promo.ActivationSettings;
import com.mapswithme.maps.promo.PromocodeActivationDialog;
import com.mapswithme.maps.search.SearchController;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.state.SuppotedState;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.ShareAction;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.Yota;
import com.mapswithme.util.statistics.Statistics;
import com.nvidia.devtech.NvEventQueueActivity;

public class MWMActivity extends NvEventQueueActivity implements LocationService.Listener, OnBalloonListener, DrawerListener
{
  private final static String TAG = "MWMActivity";
  public static final String EXTRA_TASK = "map_task";

  private static final int PRO_VERSION_DIALOG = 110001;
  private static final String PRO_VERSION_DIALOG_MSG = "pro_version_dialog_msg";
  private static final int PROMO_DIALOG = 110002;

  private MWMApplication mApplication = null;
  private BroadcastReceiver m_externalStorageReceiver = null;
  private AlertDialog m_storageDisconnectedDialog = null;

  private ImageButton mLocationButton;
  private SurfaceView mMapSurface;
  private View mYopItButton;


  // for API
  private View mTitleBar;
  private ImageView mAppIcon;
  private TextView mAppTitle;
  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<MWMActivity.MapTask>();


  // Drawer components
  private DrawerLayout mDrawerLayout;
  private View mMainDrawer;


  // search
  private SearchController mSearchController;

  private String mProDialogMessage;

  public native void deactivatePopup();

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
            : ButtonState.WAITING_LOCATION, mLocationButton);
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
        checkFacebookDialog();
        checkBuyProDialog();
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

  public void onBookmarksClicked()
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

  private boolean m_needCheckUpdate = true;

  private boolean mRenderingInitialized = false;


  private void checkUpdateMaps()
  {
    // do it only once
    if (m_needCheckUpdate)
    {
      m_needCheckUpdate = false;

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
      // Trying to find package with installed Facebook application.
      // Exception is thrown if we don't have one.
      getPackageManager().getPackageInfo("com.facebook.katana", 0);

      // Profile id is taken from http://graph.facebook.com/MapsWithMe
      startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("fb://profile/111923085594432")));
    }
    catch (final Exception e)
    {
      // Show Facebook page in browser.
      startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.facebook.com/MapsWithMe")));
    }
  }

  private boolean isChinaISO(String iso)
  {
    final String arr[] = { "CN", "CHN", "HK", "HKG", "MO", "MAC" };
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
    if ((ConnectionState.getState(this) != ConnectionState.NOT_CONNECTED) &&
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
      });
    }
  }

  private void showProVersionBanner(final String message)
  {
    mProDialogMessage = message;
    runOnUiThread(new Runnable()
    {

      @SuppressWarnings("deprecation")
      @Override
      public void run()
      {
        showDialog(PRO_VERSION_DIALOG);
      }
    });
  }

  private void runProVersionMarketActivity()
  {
    try
    {
      startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(mApplication.getProVersionURL())));
    }
    catch (final Exception e1)
    {
      try
      {
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(mApplication.getDefaultProVersionURL())));
      }
      catch (final Exception e2)
      {
        /// @todo Probably we should show some alert toast here?
        Log.w(TAG, "Can't run activity" + e2);
      }
    }
  }

  private void checkBuyProDialog()
  {
    if (!mApplication.isProVersion() &&
        (ConnectionState.getState(this) != ConnectionState.NOT_CONNECTED) &&
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
          runProVersionMarketActivity();
        }
      });
    }
  }

  private void runSearchActivity()
  {
    startActivity(new Intent(this, SearchActivity.class));
  }

  public void onSearchClicked()
  {
    if (!(mApplication.isProVersion() || ActivationSettings.isSearchActivated(this)))
    {
      showProVersionBanner(getString(R.string.search_available_in_pro_version));
    }
    else
    {
      if (!getMapStorage().updateMaps(R.string.search_update_maps, this, new MapStorage.UpdateFunctor()
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
  }

  @Override
  public boolean onSearchRequested()
  {
    onSearchClicked();
    return false;
  }

  private void runDownloadActivity()
  {
    startActivity(new Intent(this, DownloadUI.class));
  }

  public void onDownloadClicked()
  {
    runDownloadActivity();
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
    setContentView(R.layout.activity_map);
    super.onCreate(savedInstanceState);
    mApplication = (MWMApplication)getApplication();

    // Do not turn off the screen while benchmarking
    if (mApplication.nativeIsBenchmarking())
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    nativeConnectDownloadButton();

    //set up view
    mLocationButton = (ImageButton) findViewById(R.id.map_button_myposition);
    mTitleBar = findViewById(R.id.title_bar);
    mAppIcon = (ImageView) findViewById(R.id.app_icon);
    mAppTitle = (TextView) findViewById(R.id.app_title);
    mMapSurface = (SurfaceView) findViewById(R.id.map_surfaceview);

    setUpDrawer();
    yotaSetup();
    alignControls();

    Framework.connectBalloonListeners(this);

    final Intent intent = getIntent();
    // We need check for tasks both in onCreate and onNewIntent
    // because of bug in OS: https://code.google.com/p/android/issues/detail?id=38629
    addTask(intent);

    // Initialize location service
    getLocationService();

    mSearchController = SearchController.get();
    mSearchController.onCreate(this);
  }

  private void setUpDrawer()
  {
    final boolean isPro = mApplication.isProVersion();
    final OnClickListener drawerItemsClickListener = new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        final int id = v.getId();

        if (R.id.menuitem_settings_activity == id)
          ContextMenu.onItemSelected(R.id.menuitem_settings_activity, MWMActivity.this);
        else if (R.id.map_container_bookmarks == id)
        {
          if (isPro || Yota.isYota())
            onBookmarksClicked();
          else
            runProVersionMarketActivity();
        }
        else if (R.id.map_container_download == id)
        {
          onDownloadClicked();
        }
        else if (R.id.buy_pro == id)
        {
          runProVersionMarketActivity();
        }
        else if (R.id.map_button_share_myposition == id)
        {
          final Location loc = MWMApplication.get().getLocationService().getLastKnown();
          if (loc != null)
          {
            final String geoUrl = Framework.getGe0Url(loc.getLatitude(), loc.getLongitude(), Framework.getDrawScale(), "");
            final String httpUrl = Framework.getHttpGe0Url(loc.getLatitude(), loc.getLongitude(), Framework.getDrawScale(), "");
            final String body = getString(R.string.my_position_share_sms, geoUrl, httpUrl);
            // we use shortest message we can have here
            ShareAction.getAnyShare().shareWithText(getActivity(), body,"");
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
              }})
            .create().show();
          }
        }

        toggleDrawer();
      }
    };


    mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
    mDrawerLayout.setDrawerListener(this);
    mMainDrawer = findViewById(R.id.left_drawer);
    mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED);

    mMapSurface.setOnTouchListener(new OnTouchListener()
    {
      @Override
      public boolean onTouch(View v, MotionEvent event)
      {
        return MWMActivity.this.onTouchEvent(event);
      }
    });

    final SimpleOnGestureListener gestureListener = new SimpleOnGestureListener()
    {
      @Override
      public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
      {
        if (mDrawerLayout.isDrawerOpen(mMainDrawer))
          return false;

        final float dX = e2.getX() - e1.getX();
        if (dX < 0) // scroll is left
        {
          final float dY = e2.getY() - e1.getY();
          if (Math.abs(dX) > Math.abs(dY)) // is closer to horizontal
          {
              mDrawerLayout.openDrawer(mMainDrawer);
            return true;
          }
        }
        return false;
      }
    };

    final GestureDetectorCompat detector = new GestureDetectorCompat(this, gestureListener);
    final View toggleDrawer = findViewById(R.id.map_button_toggle_drawer);

    toggleDrawer.setOnTouchListener(new OnTouchListener()
    {

      @Override
      public boolean onTouch(View v, MotionEvent event)
      {
        return detector.onTouchEvent(event);
      }
    });

    findViewById(R.id.map_container_bookmarks).setOnClickListener(drawerItemsClickListener);
    findViewById(R.id.map_container_download).setOnClickListener(drawerItemsClickListener);
    toggleDrawer.setOnClickListener(drawerItemsClickListener);
    findViewById(R.id.menuitem_settings_activity).setOnClickListener(drawerItemsClickListener);
    findViewById(R.id.map_button_share_myposition).setOnClickListener(drawerItemsClickListener);
    findViewById(R.id.buy_pro).setOnClickListener(drawerItemsClickListener);



    final TextView bookmarksView = (TextView) findViewById(R.id.map_button_bookmarks);

    final Resources res = getResources();

    bookmarksView
      .setCompoundDrawablesWithIntrinsicBounds(res.getDrawable(R.drawable.ic_bookmarks_selector), null,
        isPro || Yota.isYota() ? null : res.getDrawable(R.drawable.ic_probadge), null);


    final Button buyProButton = (Button)findViewById(R.id.buy_pro);
    if (isPro)
      UiUtils.hide(buyProButton);
  }

  private void yotaSetup()
  {
    // yota setup
    mYopItButton = findViewById(R.id.yop_it);
    if (!Yota.isYota())
      mYopItButton.setVisibility(View.INVISIBLE);
    else
    {
      mYopItButton.setOnClickListener(new OnClickListener()
      {

        @Override
        public void onClick(View v)
        {
          final double[] latLon = Framework.getScreenRectCenter();
          final double   zoom   = Framework.getDrawScale();

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
    Framework.clearBalloonListeners();

    super.onDestroy();
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    addTask(intent);
  }

  private final static String EXTRA_CONSUMED = "mwm.extra.intent.processed";
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
    deactivatePopup();
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

    findViewById(R.id.scroll_up).setPadding(0, (int) getResources().getDimension(R.dimen.drawer_top_padding), 0, 0);
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
            }
            catch (final Exception e1)
            {
              // On older Android devices location settings are merged with security
              try
              {
                startActivity(new Intent(android.provider.Settings.ACTION_SECURITY_SETTINGS));
              }
              catch (final Exception e2)
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

  public void onCompassStatusChanged(int newStatus)
  {
    if (newStatus == 1)
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.FOLLOW_MODE, mLocationButton);
    else if (getLocationState().hasPosition())
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.HAS_LOCATION, mLocationButton);
    else
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.NO_LOCATION, mLocationButton);
  }

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

  @Override
  public void onLocationUpdated(final Location l)
  {
    if (getLocationState().isFirstPosition())
      LocationButtonImageSetter.setButtonViewFromState(ButtonState.HAS_LOCATION, mLocationButton);

    nativeLocationUpdated(l.getTime(), l.getLatitude(), l.getLongitude(), l.getAccuracy(), l.getAltitude(), l.getSpeed(), l.getBearing());
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    final double angles[] = { magneticNorth, trueNorth };
    getLocationService().correctCompassAngles(getWindowManager().getDefaultDisplay(), angles);
    nativeCompassUpdated(time, angles[0], angles[1], accuracy);
  }
  //@}

  private int m_compassStatusListenerID = -1;

  private void startWatchingCompassStatusUpdate()
  {
    m_compassStatusListenerID = mApplication.getLocationState().addCompassStatusListener(this);
  }

  private void stopWatchingCompassStatusUpdate()
  {
    mApplication.getLocationState().removeCompassStatusListener(m_compassStatusListenerID);
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

    if (SettingsActivity.isZoomButtonsEnabled(mApplication))
      UiUtils.show(findViewById(R.id.map_button_plus), findViewById(R.id.map_button_minus));
    else
      UiUtils.hide(findViewById(R.id.map_button_plus), findViewById(R.id.map_button_minus));


    mSearchController.onResume();
  }

  @Override
  public void setViewFromState(SuppotedState state)
  {
    if (state == SuppotedState.API_REQUEST && ParsedMmwRequest.hasRequest())
    {
      // show title
      mTitleBar.findViewById(R.id.up_block).setOnClickListener(new OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          onBackPressed();
        }
      });

      final ParsedMmwRequest request = ParsedMmwRequest.getCurrentRequest();
      if (request.hasTitle())
        mAppTitle.setText(request.getTitle());
      else
        mAppTitle.setText(request.getCallerName(this));

      mAppIcon.setImageDrawable(request.getIcon(this));
      mTitleBar.setVisibility(View.VISIBLE);

    }
    else
    {
      // hide title
      mTitleBar.setVisibility(View.GONE);
    }
  }

  @Override
  public void onBackPressed()
  {
    if (getState() == SuppotedState.API_REQUEST)
      getMwmApplication()
        .getAppStateManager()
        .transitionTo(SuppotedState.DEFAULT_MAP);

    super.onBackPressed();
  }

  // Initialized to invalid combination to force update on the first check
  private boolean m_storageAvailable = false;
  private boolean m_storageWriteable = true;

  private void updateExternalStorageState()
  {
    boolean available, writeable;
    final String state = Environment.getExternalStorageState();
    if (Environment.MEDIA_MOUNTED.equals(state))
    {
      available = writeable = true;
    }
    else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state))
    {
      available = true;
      writeable = false;
    }
    else
      available = writeable = false;

    if (m_storageAvailable != available || m_storageWriteable != writeable)
    {
      m_storageAvailable = available;
      m_storageWriteable = writeable;
      handleExternalStorageState(available, writeable);
    }
  }

  private void handleExternalStorageState(boolean available, boolean writeable)
  {
    if (available && writeable)
    {
      // Add local maps to the model
      nativeStorageConnected();

      // enable downloader button and dismiss blocking popup
      findViewById(R.id.map_button_download).setVisibility(View.VISIBLE);
      if (m_storageDisconnectedDialog != null)
        m_storageDisconnectedDialog.dismiss();
    }
    else if (available)
    {
      // Add local maps to the model
      nativeStorageConnected();

      // disable downloader button and dismiss blocking popup
      findViewById(R.id.map_button_download).setVisibility(View.INVISIBLE);
      if (m_storageDisconnectedDialog != null)
        m_storageDisconnectedDialog.dismiss();
    }
    else
    {
      // Remove local maps from the model
      nativeStorageDisconnected();

      // enable downloader button and show blocking popup
      findViewById(R.id.map_button_download).setVisibility(View.VISIBLE);
      if (m_storageDisconnectedDialog == null)
      {
        m_storageDisconnectedDialog = new AlertDialog.Builder(this)
        .setTitle(R.string.external_storage_is_not_available)
        .setMessage(getString(R.string.disconnect_usb_cable))
        .setCancelable(false)
        .create();
      }
      m_storageDisconnectedDialog.show();
    }
  }

  private boolean isActivityPaused()
  {
    // This receiver is null only when activity is paused (see onPause, onResume).
    return (m_externalStorageReceiver == null);
  }

  private void startWatchingExternalStorage()
  {
    m_externalStorageReceiver = new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        updateExternalStorageState();
      }
    };

    final IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_REMOVED);
    filter.addAction(Intent.ACTION_MEDIA_EJECT);
    filter.addAction(Intent.ACTION_MEDIA_SHARED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTABLE);
    filter.addAction(Intent.ACTION_MEDIA_CHECKING);
    filter.addAction(Intent.ACTION_MEDIA_NOFS);
    filter.addDataScheme("file");
    registerReceiver(m_externalStorageReceiver, filter);

    updateExternalStorageState();
  }

  @Override
  @Deprecated
  protected void onPrepareDialog(int id, Dialog dialog, Bundle args)
  {
    if (id == PRO_VERSION_DIALOG)
    {
      ((AlertDialog)dialog).setMessage(mProDialogMessage);
    }
    else
    {
      super.onPrepareDialog(id, dialog, args);
    }
  }

  @Override
  @Deprecated
  protected Dialog onCreateDialog(int id)
  {
    if (id == PRO_VERSION_DIALOG)
    {
      return new AlertDialog.Builder(getActivity())
      .setMessage("")
      .setPositiveButton(getString(R.string.get_it_now), new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          dlg.dismiss();
          runProVersionMarketActivity();
        }
      })
      .setNegativeButton(getString(R.string.cancel), new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dlg, int which)
        {
          dlg.dismiss();
        }
      })
      .setOnKeyListener(new OnKeyListener()
      {
        @Override
        public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event)
        {
          if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP)
          {
            if (ActivationSettings.isSearchActivated(getApplicationContext()))
              return false;

            showDialog(PROMO_DIALOG);
            dismissDialog(PRO_VERSION_DIALOG);
            return true;
          }
          return false;
        }
      })
      .create();
    }
    else if (id == PROMO_DIALOG)
      return new PromocodeActivationDialog(this);
    else
      return super.onCreateDialog(id);
  }

  private void stopWatchingExternalStorage()
  {
    if (m_externalStorageReceiver != null)
    {
      unregisterReceiver(m_externalStorageReceiver);
      m_externalStorageReceiver = null;
    }
  }


  private void toggleDrawer()
  {
    if (mDrawerLayout.isDrawerOpen(mMainDrawer))
      mDrawerLayout.closeDrawer(mMainDrawer);
    else
      mDrawerLayout.openDrawer(mMainDrawer);
  }

  @Override
  public boolean onKeyUp(int keyCode, KeyEvent event)
  {
    if (KeyEvent.KEYCODE_MENU == keyCode && !event.isCanceled())
    {
      toggleDrawer();
      return true;
    }
    return super.onKeyUp(keyCode, event);
  }

  ////
  //    Map TASKS
  ////

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

    public ShowCountryTask(Index index)
    {
      mIndex = index;
    }

    @Override
    public boolean run(MWMActivity target)
    {
      target.getMapStorage().showCountry(mIndex);
      return true;
    }
  }

  ////
  //   NATIVE callbacks and methods
  ////

  @Override
  public void onApiPointActivated(final double lat, final double lon, final String name, final String id)
  {
    if (ParsedMmwRequest.hasRequest())
    {
      final ParsedMmwRequest request = ParsedMmwRequest.getCurrentRequest();
      request.setPointData(lat, lon, name, id);

      if (request.doReturnOnBalloonClick())
      {
        request.sendResponseAndFinish(this, true);
        // we dont show MapObject in this case
        return;
      }
    }

    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        MapObjectActivity.startWithApiPoint(getActivity(), name, null, null, lat, lon);
      }
    });
  }

  @Override
  public void onPoiActivated(final String name, final String type, final String address, final double lat, final double lon)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        MapObjectActivity.startWithPoi(getActivity(), name, type, address, lat, lon);
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
        MapObjectActivity.startWithBookmark(getActivity(), category, bookmarkIndex);
      }
    });
  }

  @Override
  public void onMyPositionActivated(final double lat, final double lon)
  {
    runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        MapObjectActivity.startWithMyPosition(getActivity(), lat, lon);
      }
    });
  }

  public static Intent createShowMapIntent(Context context, Index index)
  {
    return new Intent(context, DownloadResourcesActivity.class)
      .putExtra(DownloadResourcesActivity.EXTRA_COUNTRY_INDEX, index);
  }

  private native void nativeStorageConnected();
  private native void nativeStorageDisconnected();

  private native void nativeConnectDownloadButton();
  private native void nativeDownloadCountry();

  private native void nativeDestroy();

  private native void nativeOnLocationError(int errorCode);
  private native void nativeLocationUpdated(long time, double lat, double lon, float accuracy, double altitude, float speed, float bearing);
  private native void nativeCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);

  private native boolean nativeIsInChina(double lat, double lon);

  public native boolean showMapForUrl(String url);

  @Override
  public void onDrawerClosed(View arg0)
  {
    mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED);
  }

  @Override
  public void onDrawerOpened(View arg0)
  {
    mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED);
  }

  @Override
  public void onDrawerSlide(View arg0, float arg1)
  {
  }

  @Override
  public void onDrawerStateChanged(int arg0)
  {
  }
}
