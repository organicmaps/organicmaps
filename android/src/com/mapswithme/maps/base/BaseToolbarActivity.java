package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public abstract class BaseToolbarActivity extends BaseMwmFragmentActivity
{
  @CallSuper
  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    Toolbar toolbar = getToolbar();
    if (toolbar != null)
    {
      UiUtils.extendViewWithStatusBar(toolbar);
      int title = getToolbarTitle();
      if (title == 0)
        toolbar.setTitle(getTitle());
      else
        toolbar.setTitle(title);

      UiUtils.showHomeUpButton(toolbar);
      displayToolbarAsActionBar();
    }
  }

  @StringRes
  protected int getToolbarTitle()
  {
    return 0;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    throw new RuntimeException("Must be implemented in child classes!");
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.activity_fragment_and_toolbar;
  }

  @Override
  protected int getFragmentContentResId()
  {
    return R.id.fragment_container;
  }

}
