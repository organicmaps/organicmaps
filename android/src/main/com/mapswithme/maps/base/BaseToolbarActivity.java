package com.mapswithme.maps.base;

import android.os.Bundle;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public abstract class BaseToolbarActivity extends BaseMwmFragmentActivity
{
  @Nullable
  private String mLastTitle;

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

  public void stackFragment(@NonNull Class<? extends Fragment> fragmentClass,
                            @Nullable String title, @Nullable Bundle args)
  {
    final int resId = getFragmentContentResId();
    if (resId <= 0 || findViewById(resId) == null)
      throw new IllegalStateException("Fragment can't be added, since getFragmentContentResId() " +
          "isn't implemented or returns wrong resourceId.");

    String name = fragmentClass.getName();
    final Fragment fragment = Fragment.instantiate(this, name, args);
    getSupportFragmentManager().beginTransaction()
        .replace(resId, fragment, name)
        .addToBackStack(null)
        .commitAllowingStateLoss();
    getSupportFragmentManager().executePendingTransactions();

    if (title != null)
    {
      Toolbar toolbar = getToolbar();
      if (toolbar != null && toolbar.getTitle() != null)
      {
        mLastTitle = toolbar.getTitle().toString();
        toolbar.setTitle(title);
      }
    }
  }

  @Override
  public void onBackPressed()
  {
    if (mLastTitle != null)
      getToolbar().setTitle(mLastTitle);

    super.onBackPressed();
  }
}
