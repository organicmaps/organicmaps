package com.mapswithme.maps.settings;

import android.graphics.Rect;
import android.os.Bundle;
import android.support.annotation.LayoutRes;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.base.BaseMwmFragment;

abstract class BaseSettingsFragment extends BaseMwmFragment
{
  protected View mFrame;

  private final Rect mSavedPaddings = new Rect();

  protected abstract @LayoutRes int getLayoutRes();

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
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();

    restorePaddings();
  }

  protected SettingsActivity getSettingsActivity()
  {
    return (SettingsActivity) getActivity();
  }
}
