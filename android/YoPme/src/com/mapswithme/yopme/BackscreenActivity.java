package com.mapswithme.yopme;

import java.io.File;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.location.Location;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.yopme.map.MapData;
import com.mapswithme.yopme.map.MapDataProvider;
import com.mapswithme.yopme.map.MapRenderer;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;
import com.yotadevices.sdk.BSDrawer.Waveform;
import com.yotadevices.sdk.Constants.Gestures;

public class BackscreenActivity extends BSActivity
{
  public final static String EXTRA_MODE = "com.mapswithme.yopme.mode";
  public final static String EXTRA_POINT = "com.mapswithme.yopme.point";
  public final static String EXTRA_ZOOM = "com.mapswithme.yopme.zoom";
  public final static String EXTRA_LOCATION = "com.mapswithme.yopme.location";

  private final static String TAG = "YOPME";

  public enum Mode
  {
    LOCATION,
    POI,
  }

  private CharSequence mMessage;
  private Bitmap mBitmap;

  private MWMPoint mPoint;
  private Mode mMode;
  private double mZoomLevel = MapDataProvider.ZOOM_DEFAULT;
  private Location mLocation;

  protected View mView;
  protected ImageView mMapView;
  protected TextView mPoiText;
  protected TextView mWaitMessage;
  protected View mWaitScreen;

  protected MapDataProvider mMapDataProvider;

  @Override
  protected void onBSCreate()
  {
    super.onBSCreate();

    final Resources res = getResources();
    mMapDataProvider = new MapRenderer((int) res.getDimension(R.dimen.yota_width),
            (int)res.getDimension(R.dimen.yota_height));
    
    final String extStoragePath = getDataStoragePath();
    final String extTmpPath = getTempPath();

    // Create folders if they don't exist
    new File(extStoragePath).mkdirs();
    new File(extTmpPath).mkdirs();
    
    nativeInitPlatform(getApkPath(), extStoragePath, extTmpPath, "", true);

    setUpView();
  }

  @Override
  protected void onBSResume()
  {
    super.onBSResume();

    updateData();
    invalidate();
  }

  @Override
  protected void onBSRestoreInstanceState(Bundle savedInstanceState)
  {
    super.onBSRestoreInstanceState(savedInstanceState);

    if (savedInstanceState == null)
      return;

    mPoint = (MWMPoint) savedInstanceState.getSerializable(EXTRA_POINT);
    mMode  = (Mode) savedInstanceState.getSerializable(EXTRA_MODE);
    mLocation  = (Location) savedInstanceState.getParcelable(EXTRA_LOCATION);
    mZoomLevel  = savedInstanceState.getDouble(EXTRA_ZOOM);
    Log.d(TAG, "State restored.");
  }

  @Override
  protected void onBSSaveInstanceState(Bundle outState)
  {
    super.onBSSaveInstanceState(outState);

    outState.putSerializable(EXTRA_POINT, mPoint);
    outState.putSerializable(EXTRA_MODE, mMode);
    outState.putParcelable(EXTRA_LOCATION, mLocation);
    outState.putDouble(EXTRA_ZOOM, mZoomLevel);
    Log.d(TAG, "State saved.");
  }

  @Override
  protected void onBSTouchEvent(BSMotionEvent motionEvent)
  {
    super.onBSTouchEvent(motionEvent);
    final Gestures action = motionEvent.getBSAction();

    if (action == Gestures.GESTURES_BS_SINGLE_TAP)
      requestLocationUpdate();
    else if (action == Gestures.GESTURES_BS_LR)
      zoomIn();
    else if (action == Gestures.GESTURES_BS_RL)
      zoomOut();

    updateData();
    invalidate();
  }

  @Override
  protected void onHandleIntent(Intent intent)
  {
    super.onHandleIntent(intent);

    if (intent.hasExtra(LocationManager.KEY_LOCATION_CHANGED))
      onLocationUpdate((Location) intent.getParcelableExtra(LocationManager.KEY_LOCATION_CHANGED));
    else if (intent.hasExtra(EXTRA_MODE))
    {
      mMode  = (Mode) intent.getSerializableExtra(EXTRA_MODE);
      mPoint = (MWMPoint) intent.getSerializableExtra(EXTRA_POINT);

      updateData();
      invalidate();

      if (mMode == Mode.LOCATION)
        requestLocationUpdate();
    }
  }

  public void setUpView()
  {
    mView = View.inflate(this, R.layout.yota_backscreen, null);

    mMapView = (ImageView) mView.findViewById(R.id.map);
    mPoiText = (TextView) mView.findViewById(R.id.poiText);
    mWaitMessage = (TextView) mView.findViewById(R.id.waitMsg);
    mWaitScreen = mView.findViewById(R.id.waitScreen);
  }

  public void invalidate()
  {
    draw();
  }

  private void zoomIn()
  {
    if (mZoomLevel < MapDataProvider.MAX_ZOOM)
      ++mZoomLevel;
  }

  private void zoomOut()
  {
    if (mZoomLevel > MapDataProvider.MIN_ZOOM)
      --mZoomLevel;
  }

  protected void draw()
  {
    if (mView != null)
      getBSDrawer().drawBitmap(mView, Waveform.WAVEFORM_GC_FULL);
  }

  private void requestLocationUpdate()
  {
    final LocationManager lm = (LocationManager)getSystemService(Context.LOCATION_SERVICE);

    if (lm.isProviderEnabled(LocationManager.GPS_PROVIDER))
      lm.requestSingleUpdate(LocationManager.GPS_PROVIDER, BackscreenActivity.getLocationPendingIntent(this));
    else if (lm.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
      lm.requestSingleUpdate(LocationManager.NETWORK_PROVIDER, BackscreenActivity.getLocationPendingIntent(this));
    else
      throw new IllegalStateException("No providers found.");

    showWaitMessage("Waiting for location ...");
  }

  private void onLocationUpdate(Location location)
  {
    hideWaitMessage();
    mLocation = location;

    updateData();
    invalidate();
  }

  private void showWaitMessage(CharSequence msg)
  {
    mWaitMessage.setText(msg);
    mWaitScreen.setVisibility(View.VISIBLE);

    invalidate();
  }

  private void hideWaitMessage()
  {
    mWaitScreen.setVisibility(View.GONE);

    invalidate();
  }

  public void updateData()
  {
    MapData data = null;

    if (mMode == null)
      return;

    if (mMode == Mode.LOCATION)
    {
      if (mLocation == null)
        return;
      data = mMapDataProvider.getMyPositionData(mLocation.getLatitude(),
      mLocation.getLongitude(), mZoomLevel);
    }
    else if (mMode == Mode.POI)
      data = mMapDataProvider.getPOIData(mPoint, mZoomLevel);

    mBitmap = data.getBitmap();
    mMessage = data.getPoint().getName();

    mMapView.setImageBitmap(mBitmap);
    mPoiText.setText(mMessage);
  }

  public static void startInMode(Context context, Mode mode, MWMPoint point)
  {
    final Intent i = new Intent(context, BackscreenActivity.class).putExtra(EXTRA_MODE, mode).putExtra(EXTRA_POINT,
        point);

    context.startService(i);
  }

  public static PendingIntent getLocationPendingIntent(Context context)
  {
    final Intent i = new Intent(context, BackscreenActivity.class);
    final PendingIntent pi = PendingIntent.getService(context, 0, i, PendingIntent.FLAG_UPDATE_CURRENT);
    return pi;
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

  public String getDataStoragePath()
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
                           boolean isPro);
}
