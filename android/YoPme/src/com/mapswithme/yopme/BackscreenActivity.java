package com.mapswithme.yopme;

import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.yopme.bs.Backscreen;
import com.mapswithme.yopme.bs.MyLocationBackscreen;
import com.mapswithme.yopme.bs.PoiLocationBackscreen;
import com.yotadevices.sdk.BSActivity;
import com.yotadevices.sdk.BSMotionEvent;

public class BackscreenActivity extends BSActivity
{
  private Backscreen mBackscreenView = Backscreen.Stub.get();

  public final static String EXTRA_MODE = "com.mapswithme.yopme.mode";

  public enum Mode
  {
    LOCATION,
    POI,
  }

  @Override
  protected void onBSCreate()
  {
    super.onBSCreate();
    mBackscreenView.onCreate();
  }

  @Override
  protected void onBSResume()
  {
    super.onBSResume();
    mBackscreenView.onResume();
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
      switch ((Mode)intent.getSerializableExtra(EXTRA_MODE))
      {
        case LOCATION:
          mBackscreenView = new MyLocationBackscreen(this);
          break;
        case POI:
          mBackscreenView = new PoiLocationBackscreen(this, new MWMPoint(0, 0, "WTF"));
          break;
        default:
          throw new IllegalStateException();
      }
      mBackscreenView.invalidate();
    }
  }

  public static void startInMode(Context context, Mode mode)
  {
    final Intent i = new Intent(context, BackscreenActivity.class)
      .putExtra(EXTRA_MODE, mode);
    context.startService(i);
  }

}
