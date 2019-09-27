package com.mapswithme.maps.base;

import android.os.Bundle;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;
import androidx.appcompat.widget.Toolbar;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public abstract class BaseToolbarActivity extends BaseMwmFragmentActivity
{
  @CallSuper
  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);

    Toolbar toolbar = getToolbar();
    if (toolbar != null)
    {
      UiUtils.extendViewWithStatusBar(toolbar);
      int title = getToolbarTitle();
      if (title == 0)
        toolbar.setTitle(getTitle());
      else
        toolbar.setTitle(title);

      setupHomeButton(toolbar);
      displayToolbarAsActionBar();
    }
  }

  protected void setupHomeButton(@NonNull Toolbar toolbar)
  {
    UiUtils.showHomeUpButton(toolbar);
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
