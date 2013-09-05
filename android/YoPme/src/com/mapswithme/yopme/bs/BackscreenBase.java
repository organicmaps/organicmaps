package com.mapswithme.yopme.bs;

import com.mapswithme.yopme.BackscreenActivity;
import com.mapswithme.yopme.R;
import com.mapswithme.yopme.State;
import com.mapswithme.yopme.map.MapDataProvider;
import com.mapswithme.yopme.map.MapRenderer;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSDrawer;
import com.yotadevices.sdk.BSMotionEvent;
import com.yotadevices.sdk.BSDrawer.Waveform;
import com.yotadevices.sdk.Constants.Gestures;

import android.content.Context;
import android.graphics.Bitmap;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public abstract class BackscreenBase implements LocationListener
{
  protected View mView;
  protected ImageView mMapView;
  protected TextView mPoiText;
  protected TextView mWaitMessage;
  protected View mWaitScreen;

  protected BSActivity mBsActivity;
  protected int mResId;

  protected MapDataProvider mMapDataProvider;
  protected State mState;
  protected double mZoomLevel = MapDataProvider.ZOOM_DEFAULT;

  public BackscreenBase(BSActivity bsActivity, State state)
  {
    mBsActivity = bsActivity;
    mState = state;
    mResId = R.layout.yota_backscreen;
    //TODO: you know what to do with mocks
    mMapDataProvider = MapRenderer.GetRenderer();

    setUpView();
    updateView(state);
  };

  public BackscreenBase(BSActivity bsActivity, int viewResId, State state)
  {
    this(bsActivity, state);
    mResId = viewResId;
  };

  public void onCreate()
  {
  }

  public void onResume()
  {
    invalidate();
  }

  public void setUpView()
  {
    mView = View.inflate(mBsActivity, mResId, null);
    mMapView = (ImageView) mView.findViewById(R.id.map);
    mPoiText = (TextView) mView.findViewById(R.id.poiText);
    mWaitMessage = (TextView) mView.findViewById(R.id.waitMsg);
    mWaitScreen = mView.findViewById(R.id.waitScreen);
  }

  protected void updateView(State state)
  {
    updateView(state.getBitmap(),
        state.getPoint() != null ? state.getPoint().getName() : "");
  }

  protected void updateView(Bitmap backscreenBitmap, CharSequence text)
  {
    mMapView.setImageBitmap(backscreenBitmap);
    mPoiText.setText(text);
  }

  public void invalidate()
  {
    draw();
  }

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
    final Context context = mBsActivity.getApplicationContext();
    final LocationManager lm = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);

    if (lm.isProviderEnabled(LocationManager.GPS_PROVIDER))
      lm.requestSingleUpdate(LocationManager.GPS_PROVIDER, BackscreenActivity.getLocationPendingIntent(context));
    else if (lm.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
      lm.requestSingleUpdate(LocationManager.NETWORK_PROVIDER, BackscreenActivity.getLocationPendingIntent(context));
    else
      throw new IllegalStateException("No providers found.");

    showWaitMessage("Waiting for location ...");
  }

  @Override
  public void onLocationChanged(Location location)
  {
    hideWaitMessage();
  }

  public void showWaitMessage(CharSequence msg)
  {
    mWaitMessage.setText(msg);
    mWaitScreen.setVisibility(View.VISIBLE);
  }

  public void hideWaitMessage()
  {
    mWaitScreen.setVisibility(View.GONE);
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
