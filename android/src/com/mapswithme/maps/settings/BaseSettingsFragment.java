package com.mapswithme.maps.settings;

import android.app.Fragment;
import android.graphics.Rect;
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

  private final Rect mSavedPaddings = new Rect();

  protected abstract @LayoutRes int getLayoutRes();
  protected abstract BaseShadowController createShadowController();

  private void savePaddings()
  {
    View parent = (View)mFrame.getParent();
    mSavedPaddings.set(parent.getPaddingLeft(), parent.getPaddingTop(), parent.getPaddingRight(), parent.getPaddingBottom());
  }

  protected void clearPaddings()
  {
    ((View)mFrame.getParent()).setPadding(0, 0, 0, 0);
  }

  protected void restorePaddings()
  {
    ((View)mFrame.getParent()).setPadding(mSavedPaddings.left, mSavedPaddings.top, mSavedPaddings.right, mSavedPaddings.bottom);
  }

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

    savePaddings();
    if (((PreferenceActivity)getActivity()).onIsMultiPane())
    {
      mShadowController = createShadowController();
      if (mShadowController != null)
        mShadowController.attach();
    }
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();

    restorePaddings();
    if (mShadowController != null)
      mShadowController.detach();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    org.alohalytics.Statistics.logEvent("$onResume", getClass().getSimpleName() + ":" +
                                                     UiUtils.deviceOrientationAsString(getActivity()));
  }

  @Override
  public void onPause()
  {
    super.onPause();
    org.alohalytics.Statistics.logEvent("$onPause", getClass().getSimpleName() + ":" +
                                                    UiUtils.deviceOrientationAsString(getActivity()));
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
