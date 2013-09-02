package com.mapswithme.yopme.bs;

import com.mapswithme.yopme.R;
import com.mapswithme.yopme.map.MapData;
import com.mapswithme.yopme.map.MapDataProvider;
import com.mapswithme.yopme.map.MockMapDataProvider;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSDrawer;
import com.yotadevices.sdk.BSMotionEvent;
import com.yotadevices.sdk.BSDrawer.Waveform;
import com.yotadevices.sdk.Constants.Gestures;

import android.content.Context;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public abstract class BackscreenBase implements Backscreen, LocationListener
{
  protected View mView;
  protected ImageView mMapView;
  protected TextView mPoiText;

  protected BSActivity mBsActivity;
  protected int mResId;

  protected MapDataProvider mMapDataProvider;
  protected MapData mMapData;
  protected double mZoomLevel = MapDataProvider.MAX_DEFAULT;

  public BackscreenBase(BSActivity bsActivity)
  {
    mBsActivity = bsActivity;
    mResId = R.layout.yota_backscreen;
    //TODO: you know what to do with mocks
    mMapDataProvider = new MockMapDataProvider(bsActivity);
    setUpView();
  };

  public BackscreenBase(BSActivity bsActivity, int viewResId)
  {
    this(bsActivity);
    mResId = viewResId;
  };

  @Override
  public void onCreate()
  {
  }

  @Override
  public void onResume()
  {
  }

  @Override
  public void setUpView()
  {
    mView = View.inflate(mBsActivity, mResId, null);
    mMapView = (ImageView) mView.findViewById(R.id.map);
    mPoiText = (TextView) mView.findViewById(R.id.poiText);
  }

  protected void updateData(MapData mapData)
  {
    mMapView.setImageBitmap(mapData.getBitmap());
    mPoiText.setText(mapData.getPoint().getName());
  }

  @Override
  public void invalidate()
  {
    draw();
  }

  @Override
  public void onMotionEvent(BSMotionEvent motionEvent)
  {
    final Gestures action = motionEvent.getBSAction();

    if (action == Gestures.GESTURES_BS_SINGLE_TAP)
      requestLocationUpdate();
    else if (action == Gestures.GESTURES_BS_LR)
      zoomIn();
    else if (action == Gestures.GESTURES_BS_RL)
      zoomOut();
  }

  public void zoomIn()
  {
    if (mZoomLevel < MapDataProvider.MAX_ZOOM)
      ++mZoomLevel;
  }

  public void zoomOut()
  {
    if (mZoomLevel > MapDataProvider.MIN_ZOOM)
      --mZoomLevel;
  }

  @Override
  public void draw(BSDrawer drawer)
  {
    if (mView != null)
      drawer.drawBitmap(mView, Waveform.WAVEFORM_GC_FULL);
  }

  protected void draw()
  {
    draw(mBsActivity.getBSDrawer());
  }

  protected void requestLocationUpdate()
  {
    final LocationManager lm = (LocationManager)mBsActivity.getSystemService(Context.LOCATION_SERVICE);

    if (lm.isProviderEnabled(LocationManager.GPS_PROVIDER))
      lm.requestSingleUpdate(LocationManager.GPS_PROVIDER, this, null);
    else if (lm.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
      lm.requestSingleUpdate(LocationManager.NETWORK_PROVIDER, this, null);
    else
      throw new IllegalStateException("No providers found.");
  }

  @Override
  public void onProviderDisabled(String provider)
  {
  }

  @Override
  public void onProviderEnabled(String provider)
  {
  }

  @Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
  }
}
