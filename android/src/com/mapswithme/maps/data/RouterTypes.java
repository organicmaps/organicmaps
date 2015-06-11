package com.mapswithme.maps.data;

import android.content.Context;
import android.content.SharedPreferences;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;

public class RouterTypes
{
  private static final String KEY_ROUTER = "Router";

  public static final String ROUTER_PEDESTRIAN = "pedestrian";
  public static final String ROUTER_VEHICLE = "vehicle";

  private RouterTypes() {}

  public static void saveRouterType(String routerType)
  {
    final MWMApplication application = MWMApplication.get();
    final SharedPreferences prefs = application.getSharedPreferences(application.getString(R.string.pref_file_name), Context.MODE_PRIVATE);
    prefs.edit().putString(KEY_ROUTER, routerType).commit();
  }

  public static String getRouterType()
  {
    final MWMApplication application = MWMApplication.get();
    return application.getSharedPreferences(application.getString(R.string.pref_file_name), Context.MODE_PRIVATE).getString(KEY_ROUTER, ROUTER_VEHICLE);
  }
}
