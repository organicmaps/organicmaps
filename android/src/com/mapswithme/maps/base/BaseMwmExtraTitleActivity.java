package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.v7.widget.Toolbar;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public abstract class BaseMwmExtraTitleActivity extends BaseMwmFragmentActivity
{
  protected static final String EXTRA_TITLE = "activity_title";

  @Override
  @CallSuper
  protected void safeOnCreate(Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);

    String title = "";
    Bundle bundle = getIntent().getExtras();
    if (bundle != null)
    {
      title = bundle.getString(EXTRA_TITLE);
    }
    Toolbar toolbar = getToolbar();
    UiUtils.extendViewWithStatusBar(toolbar);
    toolbar.setTitle(title);
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();
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
