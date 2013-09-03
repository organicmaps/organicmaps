package com.mapswithme.yopme;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.location.LocationManager;
import com.mapswithme.yopme.bs.BackscreenBase;
import com.mapswithme.yopme.bs.MyLocationBackscreen;
import com.mapswithme.yopme.bs.PoiLocationBackscreen;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;

public class BackscreenActivity extends BSActivity
{
  private BackscreenBase mBackscreenView;

  public final static String EXTRA_MODE = "com.mapswithme.yopme.mode";

  public enum Mode
  {
    NONE,
    LOCATION,
    POI,
  }

  @Override
  protected void onBSCreate()
  {
    super.onBSCreate();
    restore();
    mBackscreenView.onCreate();
  }

  @Override
  protected void onBSResume()
  {
    super.onBSResume();
    mBackscreenView.onResume();
  }

  private void restore()
  {
    final State state = State.read(this);
    if (state != null)
    {
      if (Mode.LOCATION == state.getMode())
        mBackscreenView = new MyLocationBackscreen(this, state);
      else if (Mode.POI == state.getMode())
        mBackscreenView = new PoiLocationBackscreen(this, state);
    }
    // TODO: what to do with null?
  }

  @Override
  protected void onBSTouchEvent(BSMotionEvent motionEvent)
  {
    super.onBSTouchEvent(motionEvent);
    mBackscreenView.onMotionEvent(motionEvent);
  }

  @Override
  protected void onHandleIntent(Intent intent)
  {
    super.onHandleIntent(intent);

    if (intent.hasExtra(EXTRA_MODE))
    {
      restore();
      mBackscreenView.invalidate();
    }
    else if (intent.hasExtra(LocationManager.KEY_LOCATION_CHANGED))
      mBackscreenView.onLocationChanged((Location)intent.getParcelableExtra(LocationManager.KEY_LOCATION_CHANGED));
  }

  public static void startInMode(Context context, Mode mode)
  {
    final Intent i = new Intent(context, BackscreenActivity.class)
      .putExtra(EXTRA_MODE, mode);
    context.startService(i);
  }

  public static PendingIntent getLocationPendingIntent(Context context)
  {
    final Intent i = new Intent(context, BackscreenActivity.class);
    final PendingIntent pi = PendingIntent.getService(context, 0, i, PendingIntent.FLAG_UPDATE_CURRENT);
    return pi;
  }

}
