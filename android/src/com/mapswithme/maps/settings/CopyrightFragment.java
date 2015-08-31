package com.mapswithme.maps.settings;

import android.app.Fragment;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.R;
import com.mapswithme.maps.WebContainerDelegate;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.widget.BaseShadowController;
import com.mapswithme.maps.widget.ObservableWebView;
import com.mapswithme.maps.widget.WebViewShadowController;
import com.mapswithme.util.Constants;

public class CopyrightFragment extends Fragment
                            implements OnBackPressListener
{
  private WebContainerDelegate mDelegate;
  private BaseShadowController mShadowController;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View frame = inflater.inflate(R.layout.fragment_prefs_copyright, container, false);

    mDelegate = new WebContainerDelegate(frame, Constants.Url.COPYRIGHT)
    {
      @Override
      protected void doStartActivity(Intent intent)
      {
        startActivity(intent);
      }
    };

    return frame;
  }

  @Override
  public void onActivityCreated(Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);

    if (((PreferenceActivity)getActivity()).onIsMultiPane())
    {
      HelpFragment.adjustMargins(mDelegate.getWebView());
      mShadowController = new WebViewShadowController((ObservableWebView)mDelegate.getWebView()).attach();
    }
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    if (mShadowController != null)
      mShadowController.detach();
  }

  @Override
  public boolean onBackPressed()
  {
    if (!mDelegate.onBackPressed())
      ((SettingsActivity)getActivity()).switchToFragment(AboutFragment.class, R.string.about_menu_title);

    return true;
  }
}
