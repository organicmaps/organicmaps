package app.organicmaps.widget;

import android.app.Activity;
import android.view.View;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils;

public class ToolbarController
{
  @Nullable
  private  Activity mActivity;
  @NonNull
  private final Toolbar mToolbar;
  @NonNull
  protected final View.OnClickListener mNavigationClickListener = view -> onUpClick();

  public ToolbarController(@NonNull View root, @NonNull Activity activity)
  {
    mActivity = activity;
    mToolbar = root.findViewById(getToolbarId());


    ViewCompat.setOnApplyWindowInsetsListener(
        getToolbar(),
        WindowInsetUtils.PaddingInsetsListener.excludeBottom());

    UiUtils.setupNavigationIcon(mToolbar, mNavigationClickListener);
    setSupportActionBar(activity, mToolbar);
  }

  private void setSupportActionBar(@NonNull Activity activity, @NonNull Toolbar toolbar)
  {
    AppCompatActivity appCompatActivity = (AppCompatActivity) activity;
    appCompatActivity.setSupportActionBar(toolbar);
  }

  @IdRes
  private int getToolbarId()
  {
    return R.id.toolbar;
  }

  public void onUpClick()
  {
    Utils.navigateToParent(requireActivity());
  }

  public ToolbarController setTitle(CharSequence title)
  {
    getSupportActionBar().setTitle(title);
    return this;
  }

  public ToolbarController setTitle(@StringRes int title)
  {
    getSupportActionBar().setTitle(title);
    return this;
  }

  @SuppressWarnings("ConstantConditions")
  @NonNull
  private ActionBar getSupportActionBar()
  {
    AppCompatActivity appCompatActivity = (AppCompatActivity) mActivity;
    return appCompatActivity.getSupportActionBar();
  }

  @NonNull
  public Activity requireActivity()
  {
    if (mActivity == null)
      throw new AssertionError("Activity must be non-null!");

    return mActivity;
  }

  @NonNull
  public Toolbar getToolbar()
  {
    return mToolbar;
  }

  public void attach(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  public void detach()
  {
    mActivity = null;
  }
}
