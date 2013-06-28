package com.mapswithme.util;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.Pair;
import android.view.View;

import com.mapswithme.maps.Framework;

public final class UiUtils
{

  public static String formatLatLon(double lat, double lon)
  {
    return String.format(Locale.US, "%f, %f", lat, lon);
  }

  public static String formatLatLonToDMS(double lat, double lon)
  {
    return Framework.latLon2DMS(lat, lon);
  }

  public static Drawable setCompoundDrawableBounds(Drawable d, int dimenId, Resources res)
  {
    final int dimension = (int) res.getDimension(dimenId);
    final float aspect = (float)d.getIntrinsicWidth()/d.getIntrinsicHeight();
    d.setBounds(0, 0, (int) (aspect*dimension), dimension);
    return d;
  }

  public static void hide(View ... views)
  {
    for (View v : views)
      v.setVisibility(View.GONE);
  }

  public static void show(View ... views)
  {
    for (View v : views)
      v.setVisibility(View.VISIBLE);
  }


  public static Drawable setCompoundDrawableBounds(int drawableId, int dimenId, Resources res)
  {
    return setCompoundDrawableBounds(res.getDrawable(drawableId), dimenId, res);
  }

  public static Drawable drawCircleForPin(String type, int size, Resources res)
  {
    final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
    final Paint paint = new Paint();

    //detect color by pin name
    int color = Color.BLACK;
    try
    {
      final String[] parts = type.split("-");
      final String colorString = parts[parts.length - 1];
      color = colorByName(colorString, Color.BLACK);
    }
    catch (Exception e)
    {
      e.printStackTrace();
      // We do nothing in this case
      // Just use RED
    }
    paint.setColor(color);
    paint.setAntiAlias(true);

    final Canvas c = new Canvas(bmp);
    final float dim = size/2.0f;
    c.drawCircle(dim, dim, dim, paint);

    return new BitmapDrawable(res, bmp);
  }


  public static final int colorByName(String name, int defColor)
  {
    if (COLOR_MAP.containsKey(name.trim().toLowerCase()))
      return COLOR_MAP.get(name);
    else return defColor;
  }

  private static final Map<String, Integer> COLOR_MAP = new HashMap<String, Integer>();
  static {
    COLOR_MAP.put("blue", Color.parseColor("#33B5E5"));
    COLOR_MAP.put("brown", Color.parseColor("#A52A2A"));
    COLOR_MAP.put("green", Color.parseColor("#669900"));
    COLOR_MAP.put("orange", Color.parseColor("#FF8800"));
    COLOR_MAP.put("pink", Color.parseColor("#FFC0CB"));
    COLOR_MAP.put("purple", Color.parseColor("#9933CC"));
    COLOR_MAP.put("red", Color.parseColor("#CC0000"));
    COLOR_MAP.put("yellow", Color.parseColor("#FFFF00"));
  }

  // utility class
  private UiUtils() {};
}
