package com.mapswithme.util;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.provider.Settings;
import android.support.v4.app.FragmentActivity;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.TranslateAnimation;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.nineoldandroids.animation.Animator;

import java.util.HashMap;
import java.util.Map;

import static com.mapswithme.util.Utils.checkNotNull;

public final class UiUtils
{

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

  public static void showIf(boolean isShow, View... views)
  {
    if (isShow)
      show(views);
    else
      hide(views);
  }

  public static TranslateAnimation generateRelativeSlideAnimation(float fromX, float toX, float fromY, float toY)
  {
    return new TranslateAnimation(
        Animation.RELATIVE_TO_SELF, fromX,
        Animation.RELATIVE_TO_SELF, toX,
        Animation.RELATIVE_TO_SELF, fromY,
        Animation.RELATIVE_TO_SELF, toY);
  }

  public static TranslateAnimation generateAbsoluteSlideAnimation(float fromX, float toX, float fromY, float toY)
  {
    return new TranslateAnimation(
        Animation.ABSOLUTE, fromX,
        Animation.ABSOLUTE, toX,
        Animation.ABSOLUTE, fromY,
        Animation.ABSOLUTE, toY);
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
    } catch (final Exception e)
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
  public static int colorByName(String name, int defColor)
  {
    if (COLOR_MAP.containsKey(name.trim().toLowerCase()))
      return COLOR_MAP.get(name);
    else
      return defColor;
  }

  private static final Map<String, Integer> COLOR_MAP = new HashMap<>();

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

  public static class SimpleNineoldAnimationListener implements Animator.AnimatorListener
  {
    @Override
    public void onAnimationStart(Animator animation) {}

    @Override
    public void onAnimationEnd(Animator animation) {}

    @Override
    public void onAnimationCancel(Animator animation) {}

    @Override
    public void onAnimationRepeat(Animator animation) {}
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

  public static TextView setTextAndShow(TextView tv, CharSequence text)
  {
    checkNotNull(tv);

    tv.setText(text);
    show(tv);

    return tv;
  }

  public static TextView clearTextAndHide(TextView tv)
  {
    checkNotNull(tv);

    tv.setText(null);
    hide(tv);

    return tv;
  }

  public static void setTextAndHideIfEmpty(TextView tv, CharSequence text)
  {
    checkNotNull(tv);

    tv.setText(text);
    showIf(!TextUtils.isEmpty(text), tv);
  }

  public static void openAppInMarket(Activity activity, String marketUrl)
  {
    try
    {
      activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(marketUrl)));
    } catch (final Exception e)
    {
      e.printStackTrace();
    }
  }

  public static void showFacebookPage(Activity activity)
  {
    try
    {
      // Exception is thrown if we don't have installed Facebook application.
      activity.getPackageManager().getPackageInfo(Constants.Package.FB_PACKAGE, 0);

      activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_MAPSME_COMMUNITY_NATIVE)));
    } catch (final Exception e)
    {
      activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.FB_MAPSME_COMMUNITY_HTTP)));
    }
  }

  public static void showTwitterPage(Activity activity)
  {
    Intent intent;
    intent = new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.TWITTER_MAPSME_HTTP));
    activity.startActivity(intent);
  }

  public static String getDisplayDensityString()
  {
    final DisplayMetrics metrics = new DisplayMetrics();
    ((WindowManager) MWMApplication.get().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay().getMetrics(metrics);
    switch (metrics.densityDpi)
    {
    case DisplayMetrics.DENSITY_LOW:
      return "ldpi";
    case DisplayMetrics.DENSITY_MEDIUM:
      return "mdpi";
    case DisplayMetrics.DENSITY_HIGH:
      return "hdpi";
    case DisplayMetrics.DENSITY_XHIGH:
      return "xhdpi";
    case DisplayMetrics.DENSITY_XXHIGH:
      return "xxhdpi";
    case DisplayMetrics.DENSITY_XXXHIGH:
      return "xxxhdpi";
    }

    return "hdpi";
  }

  public static void checkConnectionAndShowAlert(final Activity activity, final String message)
  {
    if (!ConnectionState.isConnected())
    {
      activity.runOnUiThread(new Runnable()
      {
        @Override
        public void run()
        {
          new AlertDialog.Builder(activity)
              .setCancelable(false)
              .setMessage(message)
              .setPositiveButton(activity.getString(R.string.connection_settings), new DialogInterface.OnClickListener()
              {
                @Override
                public void onClick(DialogInterface dlg, int which)
                {
                  try
                  {
                    activity.startActivity(new Intent(Settings.ACTION_WIRELESS_SETTINGS));
                  } catch (final Exception ex)
                  {
                    ex.printStackTrace();
                  }

                  dlg.dismiss();
                }
              })
              .setNegativeButton(activity.getString(R.string.close), new DialogInterface.OnClickListener()
              {
                @Override
                public void onClick(DialogInterface dlg, int which)
                {
                  dlg.dismiss();
                }
              })
              .create()
              .show();
        }
      });
    }
  }

  public static void showHomeUpButton(Toolbar toolbar)
  {
    toolbar.setNavigationIcon(R.drawable.abc_ic_ab_back_mtrl_am_alpha);
  }

  public static void lockScreenOrientation(Activity activity)
  {
    if (activity.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT)
      activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    else
      activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
  }

  public static void unlockScreenOrientation(FragmentActivity activity)
  {
    activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
  }

  public static void showAlertDialog(Activity activity, int titleId)
  {
    new AlertDialog.Builder(activity)
        .setCancelable(false)
        .setMessage(titleId)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which) { dlg.dismiss(); }
        })
        .create()
        .show();
  }

  // utility class
  private UiUtils()
  {}
}
