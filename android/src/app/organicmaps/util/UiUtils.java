package app.organicmaps.util;

import android.animation.Animator;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Build;
import android.text.Html;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.AnyRes;
import androidx.annotation.AttrRes;
import androidx.annotation.ColorInt;
import androidx.annotation.DimenRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.recyclerview.widget.RecyclerView;
import com.google.android.material.textfield.TextInputLayout;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;

import java.util.Objects;

public final class UiUtils
{
  private static final int DEFAULT_TINT_COLOR = Color.parseColor("#20000000");
  public static final int NO_ID = -1;
  public static final String NEW_STRING_DELIMITER = "\n";
  public static final String PHRASE_SEPARATOR = " • ";
  public static final String WIDE_PHRASE_SEPARATOR = "  •  ";
  public static final String APPROXIMATE_SYMBOL = "~";

  public static void bringViewToFrontOf(@NonNull View frontView, @NonNull View backView)
  {
    frontView.setZ(backView.getZ() + 1);
  }

  public static void linkifyView(@NonNull View view, @IdRes int id, @StringRes int stringId,
                                 @NonNull String link)
  {
    TextView policyView = view.findViewById(id);
    Resources rs = policyView.getResources();
    policyView.setText(Html.fromHtml(rs.getString(stringId, link)));
    policyView.setMovementMethod(LinkMovementMethod.getInstance());
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
    View view = frame.findViewById(viewId);
    if (view == null)
      return;

    hide(view);
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
    View view = frame.findViewById(viewId);
    if (view == null)
      return;

    show(view);
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

  public static void hideIf(boolean condition, View... views)
  {
    if (condition)
    {
      hide(views);
    }
    else
    {
      show(views);
    }
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

  public static void clearTextAndHide(TextView tv)
  {
    tv.setText("");
    hide(tv);
  }

  public static void showHomeUpButton(Toolbar toolbar)
  {
    toolbar.setNavigationIcon(ThemeUtils.getResource(toolbar.getContext(), R.attr.homeAsUpIndicator));
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

  public static boolean isTablet(@NonNull Context context)
  {
    return MwmApplication.from(context).getResources().getBoolean(R.bool.tabletLayout);
  }

  public static int dimen(@NonNull Context context, @DimenRes int id)
  {
    return context.getResources().getDimensionPixelSize(id);
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

  public static boolean isLandscape(@NonNull Context context)
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

  public static void setFullscreen(@NonNull Activity activity, boolean fullscreen)
  {
    final Window window = activity.getWindow();
    final View decorView = window.getDecorView();
    WindowInsetsControllerCompat wic = Objects.requireNonNull(WindowCompat.getInsetsController(window, decorView));
    if (fullscreen)
    {
      wic.hide(WindowInsetsCompat.Type.systemBars());
      wic.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
    }
    else
      wic.show(WindowInsetsCompat.Type.systemBars());
  }

  public static void setupTransparentStatusBar(@NonNull Activity activity)
  {
    final Window window = activity.getWindow();
    window.getDecorView().setFitsSystemWindows(false);
    window.setStatusBarColor(Color.TRANSPARENT);
  }

  public static void setLightStatusBar(@NonNull Activity activity, boolean isLight)
  {
    final Window window = activity.getWindow();
    final View decorView = window.getDecorView();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
    {
      // It should not be possible for Window insets controller to be null
      WindowInsetsControllerCompat wic = Objects.requireNonNull(WindowCompat.getInsetsController(window, decorView));
      if (wic.isAppearanceLightStatusBars() != isLight)
        wic.setAppearanceLightStatusBars(isLight);
    }
    else
    {
      @ColorInt final int color = isLight
                                  ? ResourcesCompat.getColor(activity.getResources(), R.color.bg_statusbar_translucent, null)
                                  : Color.TRANSPARENT;
      window.setStatusBarColor(color);
    }
  }

  public static void extendViewWithStatusBar(@NonNull View view, WindowInsets windowInsets)
  {
    final int height = windowInsets.getSystemWindowInsetTop();
    final ViewGroup.LayoutParams lp = view.getLayoutParams();
    // Extend the height only when necessary
    if (lp.height != ViewGroup.LayoutParams.WRAP_CONTENT && view.getPaddingTop() < height)
    {
      lp.height += height;
      view.setLayoutParams(lp);
    }
    setViewInsetsPaddingNoBottom(view, windowInsets);
  }

  public static void setViewInsetsPadding(View view, WindowInsets windowInsets)
  {
    view.setPadding(windowInsets.getSystemWindowInsetLeft(), windowInsets.getSystemWindowInsetTop(),
                    windowInsets.getSystemWindowInsetRight(), windowInsets.getSystemWindowInsetBottom());
  }

  public static void setViewInsetsPaddingNoTop(View view, WindowInsets windowInsets)
  {
    view.setPadding(windowInsets.getSystemWindowInsetLeft(), view.getPaddingTop(),
                    windowInsets.getSystemWindowInsetRight(), windowInsets.getSystemWindowInsetBottom());
  }

  public static void setViewInsetsPaddingSides(View view, WindowInsets windowInsets)
  {
    view.setPadding(windowInsets.getSystemWindowInsetLeft(), view.getPaddingTop(),
                    windowInsets.getSystemWindowInsetRight(), view.getPaddingBottom());
  }

  public static void setViewInsetsPaddingBottom(View view, WindowInsets windowInsets)
  {
    view.setPadding(view.getPaddingLeft(), view.getPaddingTop(),
                    view.getPaddingRight(), windowInsets.getSystemWindowInsetBottom());
  }

  public static void setViewInsetsPaddingNoBottom(View view, WindowInsets windowInsets)
  {
    view.setPadding(windowInsets.getSystemWindowInsetLeft(), windowInsets.getSystemWindowInsetTop(),
                    windowInsets.getSystemWindowInsetRight(), view.getPaddingBottom());
  }

  public static void setupNavigationIcon(@NonNull Toolbar toolbar,
                                         @NonNull View.OnClickListener listener)
  {
    View customNavigationButton = toolbar.findViewById(R.id.back);
    if (customNavigationButton != null)
    {
      customNavigationButton.setOnClickListener(listener);
    }
    else
    {
      setupHomeUpButtonAsNavigationIcon(toolbar, listener);
    }
  }

  public static void setupHomeUpButtonAsNavigationIcon(@NonNull Toolbar toolbar,
                                                       @NonNull View.OnClickListener listener)
  {
    UiUtils.showHomeUpButton(toolbar);
    toolbar.setNavigationOnClickListener(listener);
  }

  public static void clearHomeUpButton(@NonNull Toolbar toolbar)
  {
    toolbar.setNavigationIcon(null);
    toolbar.setNavigationOnClickListener(null);
  }

  @AnyRes
  public static int getStyledResourceId(@NonNull Context context, @AttrRes int res)
  {
    TypedArray a = null;
    try
    {
      a = context.obtainStyledAttributes(new int[] {res});
      return a.getResourceId(0, NO_ID);
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
    parent.post(() ->
                {
                  Rect rect = new Rect();
                  view.getHitRect(rect);
                  rect.top -= extraArea;
                  rect.left -= extraArea;
                  rect.right += extraArea;
                  rect.bottom += extraArea;
                  parent.setTouchDelegate(new TouchDelegate(rect, view));
                });
  }

  public static void expandTouchAreaForViews(int extraArea, @NonNull View... views)
  {
    for (View view : views)
      expandTouchAreaForView(view, extraArea);
  }

  public static void expandTouchAreaForView(@NonNull final View view, final int top, final int left,
                                            final int bottom, final int right)
  {
    final View parent = (View) view.getParent();
    parent.post(() ->
                {
                  Rect rect = new Rect();
                  view.getHitRect(rect);
                  rect.top -= top;
                  rect.left -= left;
                  rect.right += right;
                  rect.bottom += bottom;
                  parent.setTouchDelegate(new TouchDelegate(rect, view));
                });
  }

  @ColorInt
  public static int getNotificationColor(@NonNull Context context)
  {
    return context.getResources().getColor(R.color.notification);
  }

  public static void showToastAtTop(@NonNull Context context, @StringRes int stringId)
  {
    Toast toast = Toast.makeText(context, stringId, Toast.LENGTH_LONG);
    toast.setGravity(Gravity.TOP, 0, 0);
    toast.show();
  }

  public static void showRecyclerItemView(boolean show, @NonNull View view)
  {
    if (show)
    {
      view.setLayoutParams(new RecyclerView.LayoutParams(
          ViewGroup.LayoutParams.MATCH_PARENT,
          ViewGroup.LayoutParams.WRAP_CONTENT));
      UiUtils.show(view);
    }
    else
    {
      view.setLayoutParams(new RecyclerView.LayoutParams(0, 0));
      UiUtils.hide(view);
    }
  }

  // utility class
  private UiUtils() {}
}
