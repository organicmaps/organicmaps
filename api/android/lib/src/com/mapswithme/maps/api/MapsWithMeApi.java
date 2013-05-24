
package com.mapswithme.maps.api;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.widget.Toast;

import java.util.Locale;

//TODO add javadoc for public interface
public final class MapsWithMeApi
{

  public static void showPointsOnMap(Activity caller, MWMPoint... points)
  {
    showPointsOnMap(caller, null, null, points);
  }

  public static void showPointOnMap(Activity caller, double lat, double lon, String name, String id)
  {
    showPointsOnMap(caller, (String)null, (MWMResponseHandler)null, new MWMPoint(lat, lon, name));
  }

  public static void showPointsOnMap(Activity caller, String title, MWMPoint... points)
  {
    showPointsOnMap(caller, title, null, points);
  }

  public static void showPointsOnMap(Activity caller, String title, MWMResponseHandler responseHandler, MWMPoint... points)
  {
    final Intent mwmIntent = new Intent(Const.ACTION_MWM_REQUEST);
    mwmIntent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);

    mwmIntent.putExtra(Const.EXTRA_URL, createMwmUrl(caller, title, points).toString());
    mwmIntent.putExtra(Const.EXTRA_TITLE, title);
    mwmIntent.putExtra(Const.EXTRA_CALLMEBACK_MODE, responseHandler != null);

    addCommonExtras(caller, mwmIntent);

    MWMResponseReciever.sResponseHandler = responseHandler;
    if (responseHandler != null)
    {
      // detect if it is registered
      // throw if not
      final Intent callbackIntentStub = new Intent(getCallbackAction(caller));
      final boolean recieverEnabled = caller.getPackageManager().queryBroadcastReceivers(callbackIntentStub, 0).size() > 0;
      if (!recieverEnabled)
        throw new IllegalStateException(String.format(
                  "BroadcastReciever with intent-filter for action \"%s\" must be added to manifest.", getCallbackAction(caller)));
    }

    if (isMapsWithMeInstalled(caller))
    {
      // Match activity for intent
      // TODO specify DEFAULT for Pro version.
      final ActivityInfo aInfo = caller.getPackageManager().resolveActivity(mwmIntent, 0).activityInfo;
      mwmIntent.setClassName(aInfo.packageName, aInfo.name);
      caller.startActivity(mwmIntent);
    }
    //TODO this is temporally solution, add dialog
    else 
      Toast.makeText(caller, "MapsWithMe is not installed.", Toast.LENGTH_LONG).show();
  }

  public static boolean isMapsWithMeInstalled(Context context)
  {
    final Intent intent = new Intent(Const.ACTION_MWM_REQUEST);
    return context.getPackageManager().resolveActivity(intent, 0) != null;
  }
 
  
  static StringBuilder createMwmUrl(Context context, String title, MWMPoint ... points)
  {
    StringBuilder urlBuilder = new StringBuilder("mapswithme://map?");
    // version
    urlBuilder.append("v=")
              .append(Const.API_VERSION)
              .append("&");
    // back url, always not null
    urlBuilder.append("backurl=")
              .append(getCallbackAction(context))
              .append("&");
    // title
    appendIfNotNull(urlBuilder, "appname", title);

    // points
    for (MWMPoint point : points)
    {
      if (point != null)
      { 
        urlBuilder.append("ll=")
                  .append(String.format(Locale.US, "%f,%f&", point.getLat(), point.getLon()));
        
        appendIfNotNull(urlBuilder, "n", point.getName());
        appendIfNotNull(urlBuilder, "u", point.getId());
      }
    }
    
    return urlBuilder;
  }

  static String getCallbackAction(Context context)
  {
    return Const.CALLBACK_PREFIX + context.getPackageName();
  }

  private static Intent addCommonExtras(Context context, Intent intent)
  {
    intent.putExtra(Const.EXTRA_CALLER_APP_INFO, context.getApplicationInfo());
    intent.putExtra(Const.EXTRA_API_VERSION, Const.API_VERSION);
    intent.putExtra(Const.EXTRA_CALLBACK_ACTION, getCallbackAction(context));

    return intent;
  }
  
  private static StringBuilder appendIfNotNull(StringBuilder builder, String key, String value)
  {
    if (value != null)
      builder.append(key)
             .append("=")
             .append(Uri.encode(value))
             .append("&");
    
    return builder;
  }
}
