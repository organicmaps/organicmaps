package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class AllPassSubscriptionActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return AllPassSubscriptionPagerFragment.class;
  }

  @Override
  public int getThemeResourceId(@NonNull String theme)
  {
    return R.style.MwmTheme;
  }
}
