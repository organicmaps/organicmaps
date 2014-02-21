package com.mapswithme.util;

import static com.mapswithme.util.Utils.checkNotNull;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

import android.annotation.SuppressLint;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.widget.ImageView;
import android.widget.TextView;

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
    final float aspect = (float) d.getIntrinsicWidth() / d.getIntrinsicHeight();
    d.setBounds(0, 0, (int) (aspect * dimension), dimension);
    return d;
  }

  public static void hide(View... views)
  {
    for (final View v : views)
      v.setVisibility(View.GONE);
  }

  public static void show(View... views)
  {
    for (final View v : views)
      v.setVisibility(View.VISIBLE);
  }

  public static void invisible(View... views)
  {
    for (final View v : views)
      v.setVisibility(View.INVISIBLE);
  }

  public static void hideIf(boolean condition, View... views)
  {
    if (condition)
      hide(views);
    else
      show(views);
  }

  public static void animateAndHide(final View target, Animation anim)
  {
    anim.setAnimationListener(new AnimationListener()
    {

      @Override
      public void onAnimationStart(Animation animation)
      {}

      @Override
      public void onAnimationRepeat(Animation animation)
      {}

      @Override
      public void onAnimationEnd(Animation animation)
      {
        try
        {
          hide(target);
        }
        catch (final Exception e)
        {
          // ignore
        }
      }
    });
    target.startAnimation(anim);
  }

  public static void showAndAnimate(final View target, Animation anim)
  {
    show(target);
    target.startAnimation(anim);
  }

  public static Drawable setCompoundDrawableBounds(int drawableId, int dimenId, Resources res)
  {
    return setCompoundDrawableBounds(res.getDrawable(drawableId), dimenId, res);
  }

  public static Drawable drawCircleForPin(String type, int size, Resources res)
  {
    // detect color by pin name
    int color = Color.BLACK;
    try
    {
      final String[] parts = type.split("-");
      final String colorString = parts[parts.length - 1];
      color = colorByName(colorString, Color.BLACK);
    }
    catch (final Exception e)
    {
      e.printStackTrace();
      // We do nothing in this case
      // Just use RED
    }

    return drawCircle(color, size, res);
  }

  public static Drawable drawCircle(int color, int size, Resources res)
  {
    final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
    final Paint paint = new Paint();

    paint.setColor(color);
    paint.setAntiAlias(true);

    final Canvas c = new Canvas(bmp);
    final float dim = size / 2.0f;
    c.drawCircle(dim, dim, dim, paint);

    return new BitmapDrawable(res, bmp);
  }

  @SuppressLint("DefaultLocale")
  public static final int colorByName(String name, int defColor)
  {
    if (COLOR_MAP.containsKey(name.trim().toLowerCase()))
      return COLOR_MAP.get(name);
    else
      return defColor;
  }

  private static final Map<String, Integer> COLOR_MAP = new HashMap<String, Integer>();
  static
  {
    COLOR_MAP.put("blue", Color.parseColor("#33ccff"));
    COLOR_MAP.put("brown", Color.parseColor("#663300"));
    COLOR_MAP.put("green", Color.parseColor("#66ff33"));
    COLOR_MAP.put("orange", Color.parseColor("#ff6600"));
    COLOR_MAP.put("pink", Color.parseColor("#ff33ff"));
    COLOR_MAP.put("purple", Color.parseColor("#9933ff"));
    COLOR_MAP.put("red", Color.parseColor("#ff3333"));
    COLOR_MAP.put("yellow", Color.parseColor("#ffff33"));
  }

  public static class SimpleAnimationListener implements AnimationListener
  {
    @Override
    public void onAnimationStart(Animation animation)
    {}

    @Override
    public void onAnimationEnd(Animation animation)
    {}

    @Override
    public void onAnimationRepeat(Animation animation)
    {}
  }

  public static TextView findViewSetText(View root, int textViewId, CharSequence text)
  {
    checkNotNull(root);

    final TextView tv = (TextView) root.findViewById(textViewId);
    tv.setText(text);
    return tv;
  }

  public static ImageView findImageViewSetDrawable(View root, int imageViewId, Drawable drawable)
  {
    checkNotNull(root);

    final ImageView iv = (ImageView) root.findViewById(imageViewId);
    iv.setImageDrawable(drawable);
    return iv;
  }

  public static View findViewSetOnClickListener(View root, int viewId, OnClickListener listener)
  {
    checkNotNull(root);

    final View v = root.findViewById(viewId);
    v.setOnClickListener(listener);
    return v;
  }

  // utility class
  private UiUtils()
  {};
}
