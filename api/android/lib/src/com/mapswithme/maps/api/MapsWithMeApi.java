/******************************************************************************
   Copyright (c) 2013, MapsWithMe GmbH All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list
  of conditions and the following disclaimer. Redistributions in binary form must
  reproduce the above copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided with the
  distribution. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.
******************************************************************************/
package com.mapswithme.maps.api;

import java.util.Locale;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.net.Uri;
public final class MapsWithMeApi
{

  /**
   * Most detailed level, buildings and trees are seen.
   */
  public static final double ZOOM_MAX = 19;
  /**
   * Least detailed level, continents are seen.
   */
  public static final double ZOOM_MIN = 1;


  /**
   *  Shows single point on the map.
   *
   * @param caller
   * @param lat
   * @param lon
   * @param name
   */
  public static void showPointOnMap(Activity caller, double lat, double lon, String name)
  {
    showPointsOnMap(caller, (String)null, (PendingIntent)null, new MWMPoint(lat, lon, name));
  }


  /**
   *  Shows single point on the map using specified
   *  zoom level in range from {@link MapsWithMeApi#ZOOM_MIN} to {@link MapsWithMeApi#ZOOM_MAX}.
   *
   * @param caller
   * @param lat
   * @param lon
   * @param name
   * @param zoomLevel
   */
  public static void showPointOnMap(Activity caller, double lat, double lon, String name, double zoomLevel)
  {
    showPointsOnMap(caller, (String)null, zoomLevel, (PendingIntent)null, new MWMPoint(lat, lon, name));
  }

  /**
   *  Shows set of points on the map.
   *
   * @param caller
   * @param title
   * @param points
   */
  public static void showPointsOnMap(Activity caller, String title, MWMPoint... points)
  {
    showPointsOnMap(caller, title, null, points);
  }

  /**
   *  Shows set of points on the maps
   *  and allows MapsWithMeApplication to send {@link PendingIntent} provided by client application.
   *
   * @param caller
   * @param title
   * @param pendingIntent
   * @param points
   */
  public static void showPointsOnMap(Activity caller, String title, PendingIntent pendingIntent, MWMPoint ... points)
  {
    showPointsOnMap(caller, title, -1, pendingIntent, points);
  }

  /**
   *  Detects if any version (Lite, Pro) of MapsWithMe, which supports
   *  API calls are installed on the device.
   *
   * @param context
   * @return
   */
  public static boolean isMapsWithMeInstalled(Context context)
  {
    final Intent intent = new Intent(Const.ACTION_MWM_REQUEST);
    return context.getPackageManager().resolveActivity(intent, 0) != null;
  }

  // Internal only code

  private static void showPointsOnMap(Activity caller, String title, double zoomLevel, PendingIntent pendingIntent, MWMPoint... points)
  {
    final Intent mwmIntent = new Intent(Const.ACTION_MWM_REQUEST);

    mwmIntent.putExtra(Const.EXTRA_URL, createMwmUrl(caller, title, zoomLevel, points).toString());
    mwmIntent.putExtra(Const.EXTRA_TITLE, title);

    final boolean hasIntent = pendingIntent != null;
    mwmIntent.putExtra(Const.EXTRA_HAS_PENDING_INTENT, hasIntent);
    if (hasIntent)
      mwmIntent.putExtra(Const.EXTRA_CALLER_PENDING_INTENT, pendingIntent);

    addCommonExtras(caller, mwmIntent);

    if (isMapsWithMeInstalled(caller))
    {
      // Match activity for intent
      final ActivityInfo aInfo = caller.getPackageManager().resolveActivity(mwmIntent, 0).activityInfo;
      mwmIntent.setClassName(aInfo.packageName, aInfo.name);
      caller.startActivity(mwmIntent);
    }
    else
      (new DownloadMapsWithMeDialog(caller)).show();
  }

  static StringBuilder createMwmUrl(Context context, String title, double zoomLevel, MWMPoint ... points)
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
    // zoom
    appendIfNotNull(urlBuilder, "z", isValidZoomLevel(zoomLevel) ? String.valueOf(zoomLevel) : null);

    // points
    for (MWMPoint point : points)
    {
      if (point != null)
      {
        urlBuilder.append("ll=")
                  .append(String.format(Locale.US, "%f,%f&", point.getLat(), point.getLon()));

        appendIfNotNull(urlBuilder, "n", point.getName());
        appendIfNotNull(urlBuilder, "id", point.getId());
      }
    }

    return urlBuilder;
  }

  static String getCallbackAction(Context context)
  {
    return Const.CALLBACK_PREFIX + context.getPackageName();
  }

  @SuppressLint("NewApi")
  private static Intent addCommonExtras(Context context, Intent intent)
  {
    intent.putExtra(Const.EXTRA_CALLER_APP_INFO, context.getApplicationInfo());
    intent.putExtra(Const.EXTRA_API_VERSION, Const.API_VERSION);

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

  private static boolean isValidZoomLevel(double zoom)
  {
    return zoom >= ZOOM_MIN && zoom <= ZOOM_MAX;
  }
}
