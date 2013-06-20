package com.mapswithme.util;

import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.view.View;

import java.util.Locale;

public final class UiUtils
{

  public static String formatLatLon(double lat, double lon)
  {
    return String.format(Locale.US, "%f, %f", lat, lon);
  }

  public static String formatLatLonToDMS(double lat, double lon)
  {
    // TODO add native conversion method
    return "40°26′47″N 079°58′36″W";
  }

  public static Drawable setCompoundDrawableBounds(Drawable d, int dimenId, Resources res)
  {
    final int dimension = (int) res.getDimension(dimenId);
    final float aspect = (float)d.getIntrinsicWidth()/d.getIntrinsicHeight();
    d.setBounds(0, 0, (int) (aspect*dimension), dimension);
    return d;
  }

  public static void hide(View view)
  {
    view.setVisibility(View.GONE);
  }

  public static void show(View view)
  {
    view.setVisibility(View.VISIBLE);
  }

  public static Drawable setCompoundDrawableBounds(int drawableId, int dimenId, Resources res)
  {
    return setCompoundDrawableBounds(res.getDrawable(drawableId), dimenId, res);
  }

  // utility class
  private UiUtils() {};
}
