package com.mapswithme.maps.settings;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.R;
import com.mapswithme.maps.WebContainerDelegate;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.Constants;

public class CopyrightFragment extends BaseSettingsFragment
                            implements OnBackPressListener
{
  private WebContainerDelegate mDelegate;

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_prefs_copyright;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    super.onCreateView(inflater, container, savedInstanceState);

    mDelegate = new WebContainerDelegate(mFrame, Constants.Url.COPYRIGHT)
    {
      @Override
      protected void doStartActivity(Intent intent)
      {
        startActivity(intent);
      }
    };

    return mFrame;
  }

  @Override
  public boolean onBackPressed()
  {
    if (!mDelegate.onBackPressed())
      getSettingsActivity().replaceFragment(AboutFragment.class,
                                            getString(R.string.about_menu_title), null);

    return true;
  }
}
