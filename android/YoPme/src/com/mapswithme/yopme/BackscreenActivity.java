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
import android.os.Looper;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.location.LocationRequester;
import com.mapswithme.util.Constants;
import com.mapswithme.util.log.Logger;
import com.mapswithme.yopme.map.MapData;
import com.mapswithme.yopme.map.MapDataProvider;
import com.mapswithme.yopme.map.MapRenderer;
import com.mapswithme.yopme.util.Utils;
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
  public  final static String EXTRA_IS_POI   = YOPME_AUTHORITY + ".is_poi";

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
  private boolean  mIsPoi;
  //@}

  private final Logger mLogger = StubLogger.get(); // SimpleLogger.get("MWM_DBG");

  protected View mView;
  protected ImageView mMapView;
  protected TextView mPoiText;
  protected TextView mPoiDist;
  protected TextView mWaitMessage;
  protected View mWaitScreen;
  protected View mPoiInfo;

  protected MapDataProvider mMapDataProvider;
  private LocationRequester mLocationRequester;

  private static final String EXTRA_POINT = "point";

  @Override
  protected void onBSTouchEnadle()
  {
    // subscribe for locations again when touches were enabled
    mLogger.d("onBSTouchEnadle : requestLocationUpdate");
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

    nativeInitPlatform(getApkPath(), extStoragePath, extTmpPath, "", true, Build.DEVICE.equals(Constants.DEVICE_YOTAPHONE));

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
    mLocationRequester.unregister();
  }

  @Override
  protected void onBSResume()
  {
    super.onBSResume();

    mLogger.d("onBSResume : requestLocationUpdate");

    mLocationRequester.register();
    requestLocationUpdate();
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
    mIsPoi = savedInstanceState.getBoolean(EXTRA_IS_POI);
  }

  @Override
  protected void onBSSaveInstanceState(Bundle outState)
  {
    super.onBSSaveInstanceState(outState);

    outState.putSerializable(EXTRA_POINT, mPoint);
    outState.putSerializable(EXTRA_MODE, mMode);
    outState.putDouble(EXTRA_ZOOM, mZoomLevel);
    outState.putBoolean(EXTRA_IS_POI, mIsPoi);
  }

  @Override
  protected void onBSTouchEvent(BSMotionEvent motionEvent)
  {
    super.onBSTouchEvent(motionEvent);
    final Gestures action = motionEvent.getBSAction();

    if (action == Gestures.GESTURES_BS_SINGLE_TAP)
    {
      mLogger.d("onBSTouchEvent : requestLocationUpdate");
      requestLocationUpdate();
    }
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
    else if (action == Gestures.GESTURES_BS_LRL || action == Gestures.GESTURES_BS_RLR)
    {
      if (mMode == Mode.LOCATION)
        return; // we are already here
      else
        setToLocationMode();
    }
    else
      return; // do not react on other events

    mLogger.d("onBSTouchEvent : invalidate");
    updateData();
    invalidate();
  }

  @Override
  protected void onHandleIntent(final Intent intent)
  {
    super.onHandleIntent(intent);

    final String action = intent.getAction();
    if (action != null && (ACTION_LOCATION + ACTION_SHOW_RECT).contains(action))
    {
      extractLocation(intent);
      extractZoom(intent);

      if (ACTION_LOCATION.equals(action))
        setToLocationMode();
      else if (ACTION_SHOW_RECT.equals(action))
        setToPoiMode(intent);

      mLogger.d("onHandleIntent : requestLocationUpdate");
      requestLocationUpdate();

      notifyBSUpdated();
    }

    new Handler(Looper.getMainLooper()).post(new Runnable()
    {
      @Override
      public void run()
      {
        mLogger.d("onHandleIntent : invalidate");
        updateData();
        invalidate();
      }
    });
  }

  private void notifyBSUpdated()
  {
    final RotationAlgorithm ra = RotationAlgorithm.getInstance(this);
    ra.issueStandardToastAndVibration();
    ra.turnScreenOffIfRotated(RotationAlgorithm.OPTION_NO_UNLOCK);
  }

  private void extractZoom(Intent intent)
  {
    mZoomLevel = intent.getDoubleExtra(EXTRA_ZOOM, MapDataProvider.COMFORT_ZOOM);
  }

  private void setToPoiMode(Intent intent)
  {
    mMode = Mode.POI;
    final double lat  = intent.getDoubleExtra(EXTRA_LAT, 0);
    final double lon  = intent.getDoubleExtra(EXTRA_LON, 0);
    final String name = intent.getStringExtra(EXTRA_NAME);

    mIsPoi = intent.getBooleanExtra(EXTRA_IS_POI, false);
    mPoint = new PoiPoint(lat, lon, name);
  }

  private void setToLocationMode()
  {
    mMode = Mode.LOCATION;
    mPoint = null;
    mIsPoi = false;
  }

  private void extractLocation(Intent intent)
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
    draw();
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
    final String updateIntervalStr = PreferenceManager.getDefaultSharedPreferences(this)
        .getString(getString(R.string.pref_loc_update), YopmePreference.LOCATION_UPDATE_DEFAULT);
      final long updateInterval = Long.parseLong(updateIntervalStr);

      // before requesting updates try to get last known in the first try
      if (mLocation == null)
        mLocation = mLocationRequester.getLastKnownLocation();

      // then listen to updates
      if (updateInterval == -1)
        mLocationRequester.startListening(60*1000, true);
      else
        mLocationRequester.startListening(updateInterval*1000, false);
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
  private void setDistance(float distance)
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
  }

  public void updateData()
  {
    ensureCorrectZoomLevel();

    if (mMode == null)
    {
      mLogger.e("Unknown mode");
      return;
    }

    MapData data = null;

    if (mMode == Mode.LOCATION)
    {
      if (mLocation == null)
      {
        showWaitMessage(getString(R.string.wait_msg));
        return;
      }

      data = mMapDataProvider.getMapData(this, getLocation(), mZoomLevel, null, getLocation());
    }
    else if (mMode == Mode.POI)
    {
      data = mMapDataProvider.getMapData(this, mPoint, mZoomLevel, mIsPoi ? mPoint : null, getLocation());
      calculateDistance();
    }

    final Bitmap bitmap = data.getBitmap();
    mMapView.setImageBitmap(bitmap);

    if (bitmap == null)
    {
      // that means we don't have this map
      showWaitMessage(getString(R.string.error_map_is_absent));
      return;
    }
    else
      hideWaitMessage();

    if (mPoint != null && mMode == Mode.POI)
    {
      final CharSequence text = mPoint.getName();
      mPoiText.setText(text);
      Utils.setViewVisibility(mPoiInfo, !TextUtils.isEmpty(mPoint.getName()));
    }
    else
      Utils.setViewVisibility(mPoiInfo, false);
  }

  private void ensureCorrectZoomLevel()
  {
    if (mZoomLevel < MapDataProvider.MIN_ZOOM)
      mZoomLevel = MapDataProvider.MIN_ZOOM;
    else if (mZoomLevel > MapDataProvider.MAX_ZOOM)
      mZoomLevel = MapDataProvider.MAX_ZOOM;
  }


  private void calculateDistance()
  {
    if (mLocation != null && mPoint != null)
    {
      final Location poiLoc = new Location("");
      poiLoc.setLatitude(mPoint.getLat());
      poiLoc.setLongitude(mPoint.getLon());
      final float dist = poiLoc.distanceTo(mLocation);
      setDistance(dist);
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
      return getPackageManager().getApplicationInfo(BuildConfig.APPLICATION_ID, 0).sourceDir;
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
    return getExtAppDirectoryPath(Constants.CACHE_DIR);
  }

  public String getExtAppDirectoryPath(String folder)
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.STORAGE_PATH, BuildConfig.APPLICATION_ID, folder));
  }

  private String getOBBGooglePath()
  {
    final String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
    return storagePath.concat(String.format(Constants.OBB_PATH, BuildConfig.APPLICATION_ID));
  }

  private PoiPoint getLocation()
  {
    if (mLocation == null)
      return null;
    return new PoiPoint(mLocation.getLatitude(), mLocation.getLongitude(), null);
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

    mLogger.d("onLocationChanged : invalidate");
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
