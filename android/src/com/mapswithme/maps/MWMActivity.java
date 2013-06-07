package com.mapswithme.maps;

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
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.telephony.TelephonyManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.mapswithme.maps.Framework.OnApiPointActivatedListener;
import com.mapswithme.maps.Framework.OnBookmarkActivatedListener;
import com.mapswithme.maps.Framework.OnMyPositionActivatedListener;
import com.mapswithme.maps.Framework.OnPoiActivatedListener;
import com.mapswithme.maps.api.MWMRequest;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.maps.promo.ActivationSettings;
import com.mapswithme.maps.promo.PromocodeActivationDialog;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.state.SuppotedState;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Utils;
import com.nvidia.devtech.NvEventQueueActivity;

import java.io.Serializable;
import java.util.Locale;

public class MWMActivity extends NvEventQueueActivity 
                         implements LocationService.Listener, OnApiPointActivatedListener,
                                    OnBookmarkActivatedListener, OnPoiActivatedListener, OnMyPositionActivatedListener
{
  public static final String EXTRA_TASK = "map_task";
  
  private static final int PRO_VERSION_DIALOG = 110001;
  private static final String PRO_VERSION_DIALOG_MSG = "pro_version_dialog_msg";
  private static final int PROMO_DIALOG = 110002;
  //VideoTimer m_timer;

  private static String TAG = "MWMActivity";

  private MWMApplication mApplication = null;
  private BroadcastReceiver m_externalStorageReceiver = null;
  private AlertDialog m_storageDisconnectedDialog = null;

  private ImageButton mMyPositionButton;
  private SurfaceView mMapSurface;
  // for API
  private View mTitleBar;
  private ImageView mAppIcon;
  private TextView mAppTitle;


  //showDialog(int, Bundle) available only form API 8
  private String mProDialogMessage;

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
    ImageButton v = mMyPositionButton;
    if (v != null)
    {
      final LocationState state = getLocationState();
      final boolean hasPosition = state.hasPosition();

      // check if we need to start location observing
      int resID = 0;
      if (hasPosition)
        resID = R.drawable.myposition_button_found;
      else if (state.isFirstPosition())
        resID = R.drawable.myposition_button_normal;

      if (resID != 0)
      {
        if (hasPosition && (state.getCompassProcessMode() == LocationState.COMPASS_FOLLOW))
        {
          state.startCompassFollowing();

          v.setImageResource(R.drawable.myposition_button_follow);
        }
        else
          v.setImageResource(resID);

        v.setSelected(true);

        // start observing in the end (button state can changed here from normal to found).
        resumeLocation();
      }
      else
      {
        v.setImageResource(R.drawable.myposition_button_normal);
        v.setSelected(false);
      }
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
    if (!mApplication.isProVersion())
    {
      showProVersionBanner(getString(R.string.bookmarks_in_pro_version));
    }
    else
    {
      startActivity(new Intent(this, BookmarkCategoriesActivity.class));
    }
  }

  public void onMyPositionClicked(View v)
  {
    final LocationState state = mApplication.getLocationState();
    ImageView vImage = (ImageView)v;
    if (!state.hasPosition())
    {
      if (!state.isFirstPosition())
      {
        // If first time pressed - start location observing:

        // Set the button state to "searching" first ...
        vImage.setImageResource(R.drawable.myposition_button_normal);
        vImage.setSelected(true);

        // ... and then call startLocation, as there could be my_position button
        // state changes in the startLocation.
        startLocation();
        return;
      }
    }
    else
    {
      if (!state.isCentered())
      {
        state.animateToPositionAndEnqueueLocationProcessMode(LocationState.LOCATION_CENTER_ONLY);
        vImage.setSelected(true);
        return;
      }
      else
        if (mApplication.isProVersion())
        {
          // Check if we need to start compass following.
          if (state.hasCompass())
          {
            if (state.getCompassProcessMode() != LocationState.COMPASS_FOLLOW)
            {
              state.startCompassFollowing();

              vImage.setImageResource(R.drawable.myposition_button_follow);
              vImage.setSelected(true);
              return;
            }
            else
              state.stopCompassFollowingAndRotateMap();
          }
        }
    }

    // Turn off location search:

    // Stop location observing first ...
    stopLocation();

    // ... and then set button state to default.
    vImage.setImageResource(R.drawable.myposition_button_normal);
    vImage.setSelected(false);
  }

  private boolean m_needCheckUpdate = true;


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
    alignZoomButtons();
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
    catch (Exception e)
    {
      // Show Facebook page in browser.
      startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.facebook.com/MapsWithMe")));
    }
  }

  private boolean isChinaISO(String iso)
  {
    String arr[] = { "CN", "CHN", "HK", "HKG", "MO", "MAC" };
    for (String s : arr)
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
    catch (Exception e1)
    {
      try
      {
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(mApplication.getDefaultProVersionURL())));
      }
      catch (Exception e2)
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

  public void onSearchClicked(View v)
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
    onSearchClicked(null);
    return false;
  }

  private void runDownloadActivity()
  {
    startActivity(new Intent(this, DownloadUI.class));
  }

  public void onDownloadClicked(View v)
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

    super.onCreate(savedInstanceState);
    mApplication = (MWMApplication)getApplication();

    // Do not turn off the screen while benchmarking
    if (mApplication.nativeIsBenchmarking())
      getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    nativeConnectDownloadButton();

    //set up view
    mMyPositionButton = (ImageButton) findViewById(R.id.map_button_myposition);
    mTitleBar = findViewById(R.id.title_bar);
    mAppIcon = (ImageView) findViewById(R.id.app_icon);
    mAppTitle = (TextView) findViewById(R.id.app_title);
    mMapSurface = (SurfaceView) findViewById(R.id.map_surfaceview);

    alignZoomButtons();

    Framework.setOnBookmarkActivatedListener(this);
    Framework.setOnPoiActivatedListener(this);
    Framework.setOnMyPositionActivatedListener(this);
    
    Intent intent = getIntent();
    // We need check for tasks both in onCreate and onNewIntent
    // because of bug in OS: https://code.google.com/p/android/issues/detail?id=38629
    handleTask(intent);
  }
  
  @Override
  public void onDestroy()
  {
    Framework.clearOnBookmarkActivatedListener();
    Framework.clearOnPoiActivatedListener();
    Framework.clearOnMyPositionActivatedListener();
    
    super.onDestroy();
  }
  
  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    handleTask(intent);
  }

  private void handleTask(Intent intent)
  {
    if (intent != null && intent.hasExtra(EXTRA_TASK))
    {
      MapTask mapTask = (MapTask) intent.getSerializableExtra(EXTRA_TASK);
      mapTask.run(this);
      intent.removeExtra(EXTRA_TASK);
    }
  }
  
  @Override
  protected void onStop()
  {
    deactivatePopup();
    super.onStop();
  }

  private void alignZoomButtons()
  {
    // Get screen density
    DisplayMetrics metrics = new DisplayMetrics();
    getWindowManager().getDefaultDisplay().getMetrics(metrics);

    final double k = metrics.density;
    final int offs = (int)(53 * k); // height of button + half space between buttons.
    final int margin = (int)(5 * k);

    LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                                                                 LinearLayout.LayoutParams.WRAP_CONTENT);
    lp.setMargins(margin, (metrics.heightPixels / 4) - offs, margin, margin);
    findViewById(R.id.map_button_plus).setLayoutParams(lp);
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
            catch (Exception e1)
            {
              // On older Android devices location settings are merged with security
              try
              {
                startActivity(new Intent(android.provider.Settings.ACTION_SECURITY_SETTINGS));
              }
              catch (Exception e2)
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
    {
      mMyPositionButton.setImageResource(R.drawable.myposition_button_follow);
    }
    else
    {
      if (getLocationState().hasPosition())
        mMyPositionButton.setImageResource(R.drawable.myposition_button_found);
      else
        mMyPositionButton.setImageResource(R.drawable.myposition_button_normal);
    }

    mMyPositionButton.setSelected(true);
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
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    if (getLocationState().isFirstPosition())
    {

      mMyPositionButton.setImageResource(R.drawable.myposition_button_found);
      mMyPositionButton.setSelected(true);
    }

    nativeLocationUpdated(time, lat, lon, accuracy);
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    double angles[] = { magneticNorth, trueNorth };
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
  }

  @Override
  public void setViewFromState(SuppotedState state)
  {
    final LayoutParams mapLp = mMapSurface.getLayoutParams();
    int marginTopForMap = 0;

    if (state == SuppotedState.API_REQUEST && MWMRequest.hasRequest())
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

      final MWMRequest request = MWMRequest.getCurrentRequest();
      if (request.hasTitle())
        mAppTitle.setText(request.getTitle());
      else
        mAppTitle.setText(request.getCallerName(this));

      mAppIcon.setImageDrawable(request.getIcon(this));
      mTitleBar.setVisibility(View.VISIBLE);
      Framework.setOnApiPointActivatedListener(this);

      marginTopForMap = (int) getResources().getDimension(R.dimen.abs__action_bar_default_height);
     }
    else
    {
      // hide title
      mTitleBar.setVisibility(View.GONE);
      Framework.clearOnApiPointActivatedListener();
    }
    //we use <merge> so we not sure of type here
    if (mapLp instanceof MarginLayoutParams)
    {
      ((MarginLayoutParams)mapLp).setMargins(0, marginTopForMap, 0, 0);
    }
  }

  @Override
  public void onBackPressed()
  {
    if (getState() == SuppotedState.API_REQUEST)
      getMwmApplication().getAppStateManager().transitionTo(SuppotedState.DEFAULT_MAP);

    super.onBackPressed();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    return ContextMenu.onCreateOptionsMenu(this, menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (ContextMenu.onOptionsItemSelected(this, item))
      return true;
    else
      return super.onOptionsItemSelected(item);
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

    IntentFilter filter = new IntentFilter();
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
      return target.setViewPortByUrl(mUrl);
    }
  }

  @Override
  public void onApiPointActivated(final double lat, final double lon, final String name, final String id)
  {
      MWMRequest.getCurrentRequest().setPointData(lat, lon, name, id);
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
  
  private native void nativeStorageConnected();
  private native void nativeStorageDisconnected();

  private native void nativeConnectDownloadButton();
  private native void nativeDownloadCountry();

  private native void nativeDestroy();

  private native void nativeOnLocationError(int errorCode);
  private native void nativeLocationUpdated(long time, double lat, double lon, float accuracy);
  private native void nativeCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);

  private native boolean nativeIsInChina(double lat, double lon);
  
  private native boolean setViewPortByUrl(String url);
}
