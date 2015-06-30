package com.mapswithme.util;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.support.v7.app.AlertDialog;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.nineoldandroids.animation.Animator;

import static com.mapswithme.util.Utils.checkNotNull;

public final class UiUtils
{
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

  /*
    Views after alpha animations with 'setFillAfter' on 2.3 can't become GONE, until clearAnimationAfterAlpha is called.
   */
  public static void clearAnimationAfterAlpha(View... views)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB)
      for (final View view : views)
        view.clearAnimation();
  }

  public static Drawable drawCircle(int color, int sizeResId, Resources res)
  {
    final int size = res.getDimensionPixelSize(sizeResId);
    final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);

    final Paint paint = new Paint();
    paint.setColor(color);
    paint.setAntiAlias(true);

    final Canvas c = new Canvas(bmp);
    final float radius = size / 2.0f;
    c.drawCircle(radius, radius, radius, paint);

    return new BitmapDrawable(res, bmp);
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

  public static void showAlertDialog(Activity activity, String title)
  {
    new AlertDialog.Builder(activity)
        .setCancelable(false)
        .setMessage(title)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which) { dlg.dismiss(); }
        })
        .create()
        .show();
  }

  public static String deviceOrientationAsString(Activity activity)
  {
    String rotation = activity.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT ? "|" : "-";
    switch (activity.getWindowManager().getDefaultDisplay().getRotation())
    {
    case Surface.ROTATION_0:
      rotation += "0";
      break;
    case Surface.ROTATION_90:
      rotation += "90";
      break;
    case Surface.ROTATION_180:
      rotation += "180";
      break;
    case Surface.ROTATION_270:
      rotation += "270";
      break;
    }
    return rotation;
  }

  public static boolean isSmallTablet()
  {
    return MWMApplication.get().getResources().getBoolean(R.bool.isSmallTablet);
  }

  public static boolean isBigTablet()
  {
    return MWMApplication.get().getResources().getBoolean(R.bool.isBigTablet);
  }

  /**
   * View's default getHitRect() had a bug and would not apply transforms properly.
   * More details : http://stackoverflow.com/questions/17750116/strange-view-gethitrect-behaviour
   *
   * @param v    view
   * @param rect rect
   */
  public static void getHitRect(View v, Rect rect)
  {
    rect.left = (int) com.nineoldandroids.view.ViewHelper.getX(v);
    rect.top = (int) com.nineoldandroids.view.ViewHelper.getY(v);
    rect.right = rect.left + v.getWidth();
    rect.bottom = rect.top + v.getHeight();
  }

  /**
   * Tests, whether views intercects each other in parent coordinates.
   *
   * @param firstView  base view
   * @param secondView covering view
   * @return intersects or not
   */
  public static boolean areViewsIntersecting(View firstView, View secondView)
  {
    if (firstView.getVisibility() == View.GONE)
      return false;

    final Rect baseRect = new Rect();
    final Rect testRect = new Rect();
    UiUtils.getHitRect(firstView, baseRect);
    UiUtils.getHitRect(secondView, testRect);

    return baseRect.intersect(testRect);
  }

  // utility class
  private UiUtils()
  {}
}
