package com.mapswithme.maps.api;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;

import com.mapswithme.maps.Framework;

public class ParsedMwmRequest
{

  private static volatile ParsedMwmRequest sCurrentRequest;

  // title
  private String mTitle;
  // pending intent to call back
  private PendingIntent mPendingIntent;
  // pick point mode
  private boolean mPickPoint;

  // response data
  private boolean mHasPoint;
  private double mLat;
  private double mLon;
  private double mZoomLevel;

  private String mName;
  private String mId;


  public double getLat() { return mLat; }

  public double getLon() { return mLon; }

  public String getName() { return mName;}

  public static boolean isPickPointMode() { return hasRequest() && getCurrentRequest().mPickPoint; }

  public static ParsedMwmRequest getCurrentRequest() { return sCurrentRequest; }

  public static boolean hasRequest() { return sCurrentRequest != null; }

  public static void setCurrentRequest(ParsedMwmRequest request) { sCurrentRequest = request; }

  /**
   * Build request from intent extras.
   */
  public static ParsedMwmRequest extractFromIntent(Intent data)
  {
    final ParsedMwmRequest request = new ParsedMwmRequest();

    request.mTitle = data.getStringExtra(Const.EXTRA_TITLE);
    request.mPickPoint = data.getBooleanExtra(Const.EXTRA_PICK_POINT, false);

    if (data.getBooleanExtra(Const.EXTRA_HAS_PENDING_INTENT, false))
      request.mPendingIntent = data.getParcelableExtra(Const.EXTRA_CALLER_PENDING_INTENT);

    return request;
  }

  public boolean hasTitle() { return mTitle != null; }

  public String getTitle() { return mTitle; }

  // Request data
  public boolean hasPoint()
  {
    return mHasPoint;
  }

  public void setHasPoint(boolean hasPoint) { mHasPoint = hasPoint; }

  public boolean hasPendingIntent() { return mPendingIntent != null; }

  public void setPointData(double lat, double lon, String name, String id)
  {
    mLat = lat;
    mLon = lon;
    mName = name;
    mId = id;
  }

  public boolean sendResponse(Context context, boolean success)
  {
    if (hasPendingIntent())
    {
      mZoomLevel = Framework.nativeGetDrawScale();
      final Intent i = new Intent();
      if (success)
      {
        i.putExtra(Const.EXTRA_MWM_RESPONSE_POINT_LAT, mLat)
            .putExtra(Const.EXTRA_MWM_RESPONSE_POINT_LON, mLon)
            .putExtra(Const.EXTRA_MWM_RESPONSE_POINT_NAME, mName)
            .putExtra(Const.EXTRA_MWM_RESPONSE_POINT_ID, mId)
            .putExtra(Const.EXTRA_MWM_RESPONSE_ZOOM, mZoomLevel);
      }
      try
      {
        mPendingIntent.send(context, success ? Activity.RESULT_OK : Activity.RESULT_CANCELED, i);
        return true;
      } catch (final Exception e)
      {
        e.printStackTrace();
      }
    }
    return false;
  }

  public void sendResponseAndFinish(final Activity activity, final boolean success)
  {
    activity.runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        sendResponse(activity, success);
        activity.finish();
      }
    });
  }

  /**
   * Do not use constructor externally. Use {@link ParsedMwmRequest#extractFromIntent(android.content.Intent)} instead.
   */
  private ParsedMwmRequest() {}
}
