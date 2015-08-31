package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public abstract class BaseToolbarActivity extends BaseMwmFragmentActivity
{
  @Override
  protected void onCreate(Bundle state)
  {
    super.onCreate(state);

    Toolbar toolbar = getToolbar();
    int title = getToolbarTitle();
    if (title == 0)
      toolbar.setTitle(getTitle());
    else
      toolbar.setTitle(title);

    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();
  }

  protected @StringRes int getToolbarTitle()
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
