package com.mapswithme.maps.settings;

import android.app.Fragment;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.support.annotation.LayoutRes;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.BaseShadowController;
import com.mapswithme.util.UiUtils;

abstract class BaseSettingsFragment extends Fragment
{
  protected View mFrame;
  private BaseShadowController mShadowController;

  protected abstract @LayoutRes int getLayoutRes();
  protected abstract BaseShadowController createShadowController();

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    mFrame = inflater.inflate(getLayoutRes(), container, false);
    return mFrame;
  }

  @Override
  public void onActivityCreated(Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);

    if (((PreferenceActivity)getActivity()).onIsMultiPane())
      mShadowController = createShadowController().attach();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();

    if (mShadowController != null)
      mShadowController.detach();
  }

  protected static void adjustMargins(View view)
  {
    int margin = UiUtils.dimen(R.dimen.margin_half);
    ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) view.getLayoutParams();
    lp.leftMargin = margin;
    lp.rightMargin = margin;
    view.setLayoutParams(lp);
  }
}
