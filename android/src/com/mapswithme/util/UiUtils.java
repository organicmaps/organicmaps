package com.mapswithme.util;

import android.animation.Animator;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Build;
import android.support.annotation.AnyRes;
import android.support.annotation.AttrRes;
import android.support.annotation.ColorRes;
import android.support.annotation.DimenRes;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.design.widget.TextInputLayout;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.taxi.TaxiManager;

public final class UiUtils
{
  private static final int DEFAULT_TINT_COLOR = Color.parseColor("#20000000");
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

  public static void hide(@NonNull View view)
  {
    view.setVisibility(View.GONE);
  }

  public static void hide(@NonNull View... views)
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

  public static void showIf(boolean condition, View parent, @IdRes int... viewIds)
  {
    for (@IdRes int id : viewIds)
    {
      if (condition)
        show(parent.findViewById(id));
      else
        hide(parent.findViewById(id));
    }
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
    return dimen(MwmApplication.get(), id);
  }

  public static int dimen(Context context, @DimenRes int id)
  {
    return context.getResources().getDimensionPixelSize(id);
  }

  public static int toPx(int dp)
  {
    if (sScreenDensity == 0)
      sScreenDensity = MwmApplication.get().getResources().getDisplayMetrics().density;

    return (int) (dp * sScreenDensity + 0.5);
  }

  public static void updateButton(Button button)
  {
    button.setTextColor(ThemeUtils.getColor(button.getContext(), button.isEnabled() ? R.attr.buttonTextColor
                                                                                    : R.attr.buttonTextColorDisabled));
  }

  public static void updateRedButton(Button button)
  {
    button.setTextColor(ThemeUtils.getColor(button.getContext(), button.isEnabled() ? R.attr.redButtonTextColor
                                                                                    : R.attr.redButtonTextColorDisabled));
  }

  public static void updateAccentButton(Button button)
  {
    button.setTextColor(ThemeUtils.getColor(button.getContext(), button.isEnabled() ? R.attr.accentButtonTextColor
                                                                                    : R.attr.accentButtonTextColorDisabled));
  }

  public static Uri getUriToResId(@NonNull Context context, @AnyRes int resId)
  {
    final Resources resources = context.getResources();
    return Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://"
                         + resources.getResourcePackageName(resId) + '/'
                         + resources.getResourceTypeName(resId) + '/'
                         + resources.getResourceEntryName(resId));
  }

  public static void setInputError(@NonNull TextInputLayout layout, @StringRes int error)
  {
    layout.setError(error == 0 ? null : layout.getContext().getString(error));
    layout.getEditText().setTextColor(error == 0 ? ThemeUtils.getColor(layout.getContext(), android.R.attr.textColorPrimary)
                                                 : layout.getContext().getResources().getColor(R.color.base_red));
  }

  public static boolean isLandscape(Context context)
  {
    return context.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE;
  }

  public static boolean isViewTouched(@NonNull MotionEvent event, @NonNull View view)
  {
    if (UiUtils.isHidden(view))
      return false;

    int x = (int) event.getX();
    int y = (int) event.getY();
    int[] location = new int[2];
    view.getLocationOnScreen(location);
    int viewX = location[0];
    int viewY = location[1];
    int width = view.getWidth();
    int height = view.getHeight();
    Rect viewRect = new Rect(viewX, viewY, viewX + width, viewY + height);

    return viewRect.contains(x, y);
  }

  public static int getStatusBarHeight(@NonNull Context context)
  {
    int result = 0;
    Resources res = context.getResources();
    int resourceId = res.getIdentifier("status_bar_height", "dimen", "android");
    if (resourceId > 0)
      result = res.getDimensionPixelSize(resourceId);

    return result;
  }

  public static void extendViewWithStatusBar(@NonNull View view)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
      return;

    int statusBarHeight = getStatusBarHeight(view.getContext());
    ViewGroup.LayoutParams lp = view.getLayoutParams();
    lp.height += statusBarHeight;
    view.setLayoutParams(lp);
    extendViewPaddingTop(view, statusBarHeight);
  }

  public static void extendViewPaddingWithStatusBar(@NonNull View view)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
      return;

    int statusBarHeight = getStatusBarHeight(view.getContext());
    extendViewPaddingTop(view, statusBarHeight);
  }

  private static void extendViewPaddingTop(@NonNull View view, int statusBarHeight)
  {
    view.setPadding(view.getPaddingLeft(), view.getPaddingTop() + statusBarHeight,
                    view.getPaddingRight(), view.getPaddingBottom());
  }

  public static void extendViewMarginWithStatusBar(@NonNull View view)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
      return;

    int statusBarHeight = getStatusBarHeight(view.getContext());
    ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) view.getLayoutParams();
    int margin = lp.getMarginStart();
    lp.setMargins(margin, margin + statusBarHeight, margin, margin);
    view.setLayoutParams(lp);
  }

  public static void setupStatusBar(@NonNull Activity activity)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
      return;

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
    {
      activity.getWindow().getDecorView().setSystemUiVisibility(
          View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
    }
    else
    {
      activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
    }

    View statusBarTintView = new View(activity);
    FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, getStatusBarHeight(activity));
    params.gravity = Gravity.TOP;
    statusBarTintView.setLayoutParams(params);
    statusBarTintView.setBackgroundColor(DEFAULT_TINT_COLOR);
    statusBarTintView.setVisibility(View.VISIBLE);
    ViewGroup decorViewGroup = (ViewGroup) activity.getWindow().getDecorView();
    decorViewGroup.addView(statusBarTintView);
  }

  public static void setupColorStatusBar(@NonNull Activity activity, @ColorRes int statusBarColor)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP)
      return;

    Window window = activity.getWindow();
    window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
    window.setStatusBarColor(ContextCompat.getColor(activity, statusBarColor));
  }

  public static int getCompassYOffset(@NonNull Context context)
  {
    return getStatusBarHeight(context);
  }

  @AnyRes
  public static int getStyledResourceId(Context context, @AttrRes int res)
  {
    TypedArray a = null;
    try
    {
      a = context.obtainStyledAttributes(new int[] {res});
      return a.getResourceId(0, -1);
    }
    finally
    {
      if (a != null)
        a.recycle();
    }
  }

  public static void setBackgroundDrawable(View view, @AttrRes int res)
  {
    view.setBackgroundResource(getStyledResourceId(view.getContext(), res));
  }

  public static void expandTouchAreaForView(@NonNull final View view, final int extraArea)
  {
    final View parent = (View) view.getParent();
    parent.post(new Runnable()
    {
      @Override
      public void run()
      {
        Rect rect = new Rect();
        view.getHitRect(rect);
        rect.top -= extraArea;
        rect.left -= extraArea;
        rect.right += extraArea;
        rect.bottom += extraArea;
        parent.setTouchDelegate(new TouchDelegate(rect, view));
      }
    });
  }

  // utility class
  private UiUtils() {}
}
