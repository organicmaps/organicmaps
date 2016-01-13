package com.mapswithme.util;

import android.animation.Animator;
import android.app.Activity;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.provider.Settings;
import android.support.annotation.DimenRes;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.Surface;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public final class UiUtils
{
  private static float sScreenDensity;

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

  public static class SimpleAnimatorListener implements Animator.AnimatorListener
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

  public interface OnViewMeasuredListener
  {
    void onViewMeasured(int width, int height);
  }

  public static void waitLayout(final View view, @NonNull final ViewTreeObserver.OnGlobalLayoutListener callback) {
    view.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
      @SuppressWarnings("deprecation")
      @Override
      public void onGlobalLayout() {
        // viewTreeObserver can be dead(isAlive() == false), we should get a new one here.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN)
          view.getViewTreeObserver().removeGlobalOnLayoutListener(this);
        else
          view.getViewTreeObserver().removeOnGlobalLayoutListener(this);

        callback.onGlobalLayout();
      }
    });
  }

  public static void measureView(final View frame, final OnViewMeasuredListener listener)
  {
    waitLayout(frame, new ViewTreeObserver.OnGlobalLayoutListener()
    {
      @Override
      public void onGlobalLayout()
      {
        Activity ac = (Activity) frame.getContext();
        if (ac == null || ac.isFinishing())
          return;

        listener.onViewMeasured(frame.getMeasuredWidth(), frame.getMeasuredHeight());
      }
    });
  }

  public static void hide(View view)
  {
    view.setVisibility(View.GONE);
  }

  public static void hide(View... views)
  {
    for (final View v : views)
      v.setVisibility(View.GONE);
  }

  public static void hide(View frame, @IdRes int viewId)
  {
    hide(frame.findViewById(viewId));
  }

  public static void hide(View frame, @IdRes int... viewIds)
  {
    for (final int id : viewIds)
      hide(frame, id);
  }

  public static void show(View view)
  {
    view.setVisibility(View.VISIBLE);
  }

  public static void show(View... views)
  {
    for (final View v : views)
      v.setVisibility(View.VISIBLE);
  }

  public static void show(View frame, @IdRes int viewId)
  {
    show(frame.findViewById(viewId));
  }

  public static void show(View frame, @IdRes int... viewIds)
  {
    for (final int id : viewIds)
      show(frame, id);
  }

  public static void invisible(View view)
  {
    view.setVisibility(View.INVISIBLE);
  }

  public static void invisible(View... views)
  {
    for (final View v : views)
      v.setVisibility(View.INVISIBLE);
  }

  public static void invisible(View frame, @IdRes int viewId)
  {
    invisible(frame.findViewById(viewId));
  }

  public static void invisible(View frame, @IdRes int... viewIds)
  {
    for (final int id : viewIds)
      invisible(frame, id);
  }

  public static boolean isHidden(View view)
  {
    return view.getVisibility() == View.GONE;
  }

  public static boolean isInvisible(View view)
  {
    return view.getVisibility() == View.INVISIBLE;
  }

  public static boolean isVisible(View view)
  {
    return view.getVisibility() == View.VISIBLE;
  }

  public static void visibleIf(boolean condition, View view)
  {
    view.setVisibility(condition ? View.VISIBLE : View.INVISIBLE);
  }

  public static void visibleIf(boolean condition, View... views)
  {
    if (condition)
      show(views);
    else
      invisible(views);
  }

  public static void showIf(boolean condition, View view)
  {
    view.setVisibility(condition ? View.VISIBLE : View.GONE);
  }

  public static void showIf(boolean condition, View... views)
  {
    if (condition)
      show(views);
    else
      hide(views);
  }

  public static void setTextAndShow(TextView tv, CharSequence text)
  {
    tv.setText(text);
    show(tv);
  }

  public static void setTextAndHideIfEmpty(TextView tv, CharSequence text)
  {
    tv.setText(text);
    showIf(!TextUtils.isEmpty(text), tv);
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
    toolbar.setNavigationIcon(ThemeUtils.getResource(toolbar.getContext(), R.attr.homeAsUpIndicator));
  }

  public static AlertDialog buildAlertDialog(Activity activity, int titleId)
  {
    return new AlertDialog.Builder(activity)
            .setCancelable(false)
            .setMessage(titleId)
            .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dlg, int which) { dlg.dismiss(); }
            })
            .create();
  }

  public static void showAlertDialog(Activity activity, int titleId)
  {
    buildAlertDialog(activity, titleId).show();
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

  public static boolean isTablet()
  {
    return MwmApplication.get().getResources().getBoolean(R.bool.tabletLayout);
  }

  public static int dimen(@DimenRes int id)
  {
    return MwmApplication.get().getResources().getDimensionPixelSize(id);
  }

  public static int toPx(int dp)
  {
    if (sScreenDensity == 0)
      sScreenDensity = MwmApplication.get().getResources().getDisplayMetrics().density;

    return (int) (dp * sScreenDensity + 0.5);
  }

  // utility class
  private UiUtils()
  {}
}
