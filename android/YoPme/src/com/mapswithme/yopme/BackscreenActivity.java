package com.mapswithme.yopme;

import java.io.File;
import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.NumberFormat;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.location.Location;
import android.location.LocationListener;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.location.LocationRequester;
import com.mapswithme.yopme.map.MapData;
import com.mapswithme.yopme.map.MapDataProvider;
import com.mapswithme.yopme.map.MapRenderer;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSDrawer.Waveform;
import com.yotadevices.sdk.BSMotionEvent;
import com.yotadevices.sdk.Constants.Gestures;
import com.yotadevices.sdk.utils.RotationAlgorithm;

public class BackscreenActivity extends BSActivity implements LocationListener
{
  private final static String TAG = "YOPME";

  private final static String YOPME_AUTHORITY   = "com.mapswithme.yopme";
  public  final static String ACTION_PREFERENCE = YOPME_AUTHORITY + ".preference";
  public  final static String ACTION_SHOW_RECT  = YOPME_AUTHORITY + ".show_rect";
  public  final static String ACTION_LOCATION   = YOPME_AUTHORITY + ".location";

  public  final static String EXTRA_LAT    = YOPME_AUTHORITY + ".lat";
  public  final static String EXTRA_LON    = YOPME_AUTHORITY + ".lon";
  public  final static String EXTRA_ZOOM   = YOPME_AUTHORITY + ".zoom";
  public  final static String EXTRA_NAME   = YOPME_AUTHORITY + ".name";
  public  final static String EXTRA_MODE   = YOPME_AUTHORITY + ".mode";

  public  final static String EXTRA_HAS_LOCATION   = YOPME_AUTHORITY + ".haslocation";
  public  final static String EXTRA_MY_LAT   = YOPME_AUTHORITY + ".mylat";
  public  final static String EXTRA_MY_LON   = YOPME_AUTHORITY + ".mylon";

  public enum Mode
  {
    LOCATION,
    POI,
  }

  /// @name Save to state.
  //@{
  private PoiPoint mPoint = null;
  private Mode mMode = Mode.LOCATION;
  private double mZoomLevel = MapDataProvider.ZOOM_DEFAULT;
  private Location mLocation = null;
  //@}

  protected View mView;
  protected ImageView mMapView;
  protected TextView mPoiText;
  protected TextView mPoiDist;
  protected TextView mWaitMessage;
  protected View mWaitScreen;
  protected View mPoiInfo;

  protected MapDataProvider mMapDataProvider;
  private LocationRequester mLocationRequester;

  private final Runnable mInvalidateDrawable = new Runnable()
  {
    @Override
    public void run()
    {
      draw();
    }
  };

  private final Runnable mRequestLocation = new Runnable()
  {
    @Override
    public void run()
    {
      requestLocationUpdateImpl();
    }
  };

  private final Handler mDeduplicateHandler = new Handler();
  private final static long REDRAW_MIN_INTERVAL = 333;

  private static final String EXTRA_POINT = "point";

  @Override
  protected void onBSTouchEnadle()
  {
    // subscribe for locations again
    Log.d(TAG, "LocationRequester: touches were enabled, requbscribe to location");
    requestLocationUpdate();
    super.onBSTouchEnadle();
  }

  @Override
  protected void onBSCreate()
  {
    super.onBSCreate();

    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getTempPath();

    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();

    nativeInitPlatform(getApkPath(), extStoragePath, extTmpPath, "", true, Build.DEVICE.equals("yotaphone"));

    /// !!! Create MapRenderer ONLY AFTER platform init !!!
    //final Resources res = getResources();
    //mMapDataProvider = new MapRenderer((int) res.getDimension(R.dimen.yota_width),
    //                                   (int)res.getDimension(R.dimen.yota_height));
    mMapDataProvider = MapRenderer.GetRenderer();

    setUpView();

    mLocationRequester = new LocationRequester(this);
    mLocationRequester.addListener(this);
  }

  @Override
  protected void onBSDestroy()
  {
    mLocationRequester.removeListener(this);
    super.onBSDestroy();
  }

  @Override
  protected void onBSPause()
  {
    super.onBSPause();
    mLocationRequester.stopListening();

    mDeduplicateHandler.removeCallbacks(mInvalidateDrawable);
    mDeduplicateHandler.removeCallbacks(mRequestLocation);
  }

  @Override
  protected void onBSResume()
  {
    super.onBSResume();
    requestLocationUpdate();

    updateData();
    invalidate();
  }

  @Override
  protected void onBSRestoreInstanceState(Bundle savedInstanceState)
  {
    super.onBSRestoreInstanceState(savedInstanceState);

    if (savedInstanceState == null)
      return;

    // Do not save and restore m_location! It's compared by getElapsedRealtimeNanos().

    mPoint = (PoiPoint) savedInstanceState.getSerializable(EXTRA_POINT);
    mMode  = (Mode) savedInstanceState.getSerializable(EXTRA_MODE);
    mZoomLevel  = savedInstanceState.getDouble(EXTRA_ZOOM);
  }

  @Override
  protected void onBSSaveInstanceState(Bundle outState)
  {
    super.onBSSaveInstanceState(outState);

    outState.putSerializable(EXTRA_POINT, mPoint);
    outState.putSerializable(EXTRA_MODE, mMode);
    outState.putDouble(EXTRA_ZOOM, mZoomLevel);
  }

  @Override
  protected void onBSTouchEvent(BSMotionEvent motionEvent)
  {
    super.onBSTouchEvent(motionEvent);
    final Gestures action = motionEvent.getBSAction();

    if (action == Gestures.GESTURES_BS_SINGLE_TAP)
      requestLocationUpdate();
    else if (action == Gestures.GESTURES_BS_LR || action == Gestures.GESTURES_BS_SCROLL_LEFT)
    {
      if (!zoomOut())
        return;
    }
    else if (action == Gestures.GESTURES_BS_RL || action == Gestures.GESTURES_BS_SCROLL_RIGHT)
    {
      if (!zoomIn())
        return;
    }
    else
      return; // do not react on other events

    updateData();
    invalidate();
  }

  @Override
  protected void onHandleIntent(Intent intent)
  {
    super.onHandleIntent(intent);

    final String action  = intent.getAction();
    if (action != null && (ACTION_LOCATION + ACTION_SHOW_RECT).contains(action))
    {
      if (intent.getBooleanExtra(EXTRA_HAS_LOCATION, false))
      {
        // use location from MWM
        final double myLat = intent.getDoubleExtra(EXTRA_MY_LAT, 0);
        final double myLon = intent.getDoubleExtra(EXTRA_MY_LON, 0);
        mLocation = new Location("MapsWithMe");
        mLocation.setLatitude(myLat);
        mLocation.setLongitude(myLon);

        mLocationRequester.setLocation(mLocation);
      }

      if (ACTION_LOCATION.equals(intent.getAction()))
        mMode = Mode.LOCATION;
      else if (ACTION_SHOW_RECT.equals(intent.getAction()))
      {
        mMode = Mode.POI;
        final double lat  = intent.getDoubleExtra(EXTRA_LAT, 0);
        final double lon  = intent.getDoubleExtra(EXTRA_LON, 0);
        final String name = intent.getStringExtra(EXTRA_NAME);
        mPoint = new PoiPoint(lat, lon, name);
      }
      mZoomLevel = intent.getDoubleExtra(EXTRA_ZOOM, MapDataProvider.COMFORT_ZOOM);

      requestLocationUpdate();
      RotationAlgorithm.getInstance(this).turnScreenOffIfRotated();
    }

    // Do always update data.
    // This function is called from back screen menu without any extras.

    updateData();
    invalidate();
  }

  public void setUpView()
  {
    mView = View.inflate(this, R.layout.yota_backscreen, null);

    mMapView = (ImageView) mView.findViewById(R.id.map);
    mPoiText = (TextView) mView.findViewById(R.id.poiText);
    mPoiDist = (TextView) mView.findViewById(R.id.poiDist);
    mWaitMessage = (TextView) mView.findViewById(R.id.waitMsg);
    mWaitScreen = mView.findViewById(R.id.waitScreen);
    mPoiInfo    = mView.findViewById(R.id.poiInfo);
  }

  public void invalidate()
  {
    mDeduplicateHandler.removeCallbacks(mInvalidateDrawable);
    mDeduplicateHandler.postDelayed(mInvalidateDrawable, REDRAW_MIN_INTERVAL);
  }

  private boolean zoomIn()
  {
    if (mZoomLevel < MapDataProvider.MAX_ZOOM)
    {
      ++mZoomLevel;
      return true;
    }
    return false;
  }

  private boolean zoomOut()
  {
    if (mZoomLevel > MapDataProvider.MIN_ZOOM)
    {
      --mZoomLevel;
      return true;
    }
    return false;
  }

  protected void draw()
  {
    if (mView != null)
      getBSDrawer().drawBitmap(mView, Waveform.WAVEFORM_GC_FULL);
  }

  private void requestLocationUpdate()
  {
    mDeduplicateHandler.removeCallbacks(mRequestLocation);
    mDeduplicateHandler.postDelayed(mRequestLocation, 300);
  }

  private void requestLocationUpdateImpl()
  {
    final String updateIntervalStr = PreferenceManager.getDefaultSharedPreferences(this)
        .getString(getString(R.string.pref_loc_update), YopmePreference.LOCATION_UPDATE_DEFAULT);
      final long updateInterval = Long.parseLong(updateIntervalStr);

      // before requesting updates try to get last known in the first try
      if (mLocation == null)
        mLocation = mLocationRequester.getLastKnownLocation();

      // then listen to updates
      if (updateInterval == -1)
        mLocationRequester.requestSingleUpdate(60*1000);
      else
      {
        // according to the manual, minDistance doesn't save battery life
        mLocationRequester.setMinDistance(0);
        mLocationRequester.setMinTime(updateInterval * 1000);
        mLocationRequester.setUpProviders();
        mLocationRequester.startListening();
      }
  }

  private void showWaitMessage(CharSequence msg)
  {
    mWaitMessage.setText(msg);
    mWaitScreen.setVisibility(View.VISIBLE);
  }

  private void hideWaitMessage()
  {
    mWaitScreen.setVisibility(View.GONE);
  }

  private final static NumberFormat df = DecimalFormat.getInstance();
  static
  {
    df.setRoundingMode(RoundingMode.DOWN);
  }
  private void setDistance(double distance)
  {
    if (distance < 0)
      mPoiDist.setVisibility(View.GONE);
    else
    {
      String suffix = "m";
      double div = 1;
      df.setMaximumFractionDigits(0);

      if (distance >= 1000)
      {
        suffix = "km";
        div = 1000;

        // set fraction digits only in [1..10) kilometers range
        if (distance < 10000)
          df.setMaximumFractionDigits(2);
      }

      mPoiDist.setText(df.format(distance/div) + suffix);
      mPoiDist.setVisibility(View.VISIBLE);
    }
  }

  public void updateData()
  {
    if (mZoomLevel < MapDataProvider.MIN_ZOOM)
      mZoomLevel = MapDataProvider.MIN_ZOOM;

    MapData data = null;

    if (mMode == null)
    {
      Log.d(TAG, "Unknown mode");
      return;
    }

    if (mMode == Mode.LOCATION)
    {
      mPoiInfo.setVisibility(View.GONE);
      if (mLocation == null)
      {
        showWaitMessage(getString(R.string.wait_msg));
        return;
      }
      data = mMapDataProvider
          .getMyPositionData(mLocation.getLatitude(), mLocation.getLongitude(), mZoomLevel);
    }
    else if (mMode == Mode.POI)
    {
      mPoiInfo.setVisibility(View.VISIBLE);
      if (mLocation != null)
        data = mMapDataProvider
          .getPOIData(mPoint, mZoomLevel, true, mLocation.getLatitude(), mLocation.getLongitude());
      else
        data = mMapDataProvider
          .getPOIData(mPoint, mZoomLevel, false, 0, 0);

      calculateDistance();
    }

    final Bitmap bitmap = data.getBitmap();
    mMapView.setImageBitmap(bitmap);

    if (bitmap == null)
    {
      // this means we don't have this map
      showWaitMessage(getString(R.string.error_map_is_absent));
      return;
    }
    else
      hideWaitMessage();

    if (mMode == Mode.POI)
    {
      mPoiText.setText(mPoint.getName());
      mPoiInfo.setVisibility(TextUtils.isEmpty(mPoint.getName()) ? View.GONE : View.VISIBLE);
    }
  }


  private void calculateDistance()
  {
    if (mLocation != null && mPoint != null)
    {
      final Location poiLoc = new Location("");
      poiLoc.setLatitude(mPoint.getLat());
      poiLoc.setLongitude(mPoint.getLon());
      setDistance(poiLoc.distanceTo(mLocation));
    }
  }

  public static void startInMode(Context context, Mode mode, PoiPoint point, double zoom)
  {
    final Intent i = new Intent(context, BackscreenActivity.class)
      .putExtra(EXTRA_MODE, mode)
      .putExtra(EXTRA_POINT, point)
      .putExtra(EXTRA_ZOOM, zoom >= MapDataProvider.MIN_ZOOM ? zoom : MapDataProvider.ZOOM_DEFAULT);

    context.startService(i);
  }

  public String getApkPath()
  {
    try
    {
      return getPackageManager().getApplicationInfo(getPackageName(), 0).sourceDir;
    }
    catch (final NameNotFoundException e)
    {
      return "";
    }
  }

  public static String getDataStoragePath()
  {
    return Environment.getExternalStorageDirectory().getAbsolutePath() + "/MapsWithMe/";
  }

  public String getTempPath()
  {
    // Can't use getExternalCacheDir() here because of API level = 7.
    return getExtAppDirectoryPath("cache");
  }

  public String getExtAppDirectoryPath(String folder)
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format("/Android/data/%s/%s/", getPackageName(), folder));
  }

  private String getOBBGooglePath()
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format("/Android/obb/%s/", getPackageName()));
  }

  static
  {
    System.loadLibrary("yopme");
  }

  private native void nativeInitPlatform(String apkPath, String storagePath,
                                         String tmpPath, String obbGooglePath,
                                         boolean isPro, boolean isYota);

  @Override
  public void onLocationChanged(Location l)
  {
      mLocation = l;

      updateData();
      invalidate();
  }

  @Override
  public void onProviderDisabled(String provider) {}

  @Override
  public void onProviderEnabled(String provider) {}

  @Override
  public void onStatusChanged(String provider, int status, Bundle extras) {}
}
