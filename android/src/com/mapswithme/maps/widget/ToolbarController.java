package com.mapswithme.maps.widget;

import android.app.Activity;
import android.support.annotation.IdRes;
import android.support.annotation.StringRes;
import android.support.v7.widget.Toolbar;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class ToolbarController
{
  protected final Activity mActivity;
  protected final Toolbar mToolbar;
  private final View.OnClickListener mNavigationClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View view)
    {
      onUpClick();
    }
  };

  public ToolbarController(View root, Activity activity)
  {
    mActivity = activity;
    mToolbar = root.findViewById(getToolbarId());

    if (useExtendedToolbar())
      UiUtils.extendViewWithStatusBar(mToolbar);
    setupNavigationListener();
  }

  protected boolean useExtendedToolbar()
  {
    return true;
  }

  private void setupNavigationListener()
  {
    View customNavigationButton = mToolbar.findViewById(R.id.back);
    if (customNavigationButton != null)
    {
      customNavigationButton.setOnClickListener(mNavigationClickListener);
    }
    else
    {
      UiUtils.showHomeUpButton(mToolbar);
      mToolbar.setNavigationOnClickListener(mNavigationClickListener);
    }
  }

  @IdRes
  private int getToolbarId()
  {
    return R.id.toolbar;
  }

  public void onUpClick()
  {
    Utils.navigateToParent(mActivity);
  }

  public ToolbarController setTitle(CharSequence title)
  {
    mToolbar.setTitle(title);
    return this;
  }

  public ToolbarController setTitle(@StringRes int title)
  {
    mToolbar.setTitle(title);
    return this;
  }

  public Toolbar getToolbar()
  {
    return mToolbar;
  }

  public View findViewById(@IdRes int res)
  {
    return mToolbar.findViewById(res);
  }
}
