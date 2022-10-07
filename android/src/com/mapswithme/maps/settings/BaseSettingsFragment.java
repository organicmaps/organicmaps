package com.mapswithme.maps.settings;

import android.graphics.Rect;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.LayoutRes;
import androidx.annotation.Nullable;

import com.mapswithme.maps.base.BaseMwmFragment;

abstract class BaseSettingsFragment extends BaseMwmFragment
{
  private View mFrame;

  private final Rect mSavedPaddings = new Rect();

  protected abstract @LayoutRes int getLayoutRes();

  private void savePaddings()
  {
    View parent = (View)mFrame.getParent();
    if (parent != null)
    {
      mSavedPaddings.set(parent.getPaddingLeft(), parent.getPaddingTop(), parent.getPaddingRight(), parent.getPaddingBottom());
    }
  }

  protected void clearPaddings()
  {
    View parent = (View)mFrame.getParent();
    if (parent != null)
    {
      parent.setPadding(0, 0, 0, 0);
    }
  }

  protected void restorePaddings()
  {
    View parent = (View)mFrame.getParent();
    if (parent != null)
    {
      parent.setPadding(mSavedPaddings.left, mSavedPaddings.top, mSavedPaddings.right, mSavedPaddings.bottom);
    }
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return mFrame = inflater.inflate(getLayoutRes(), container, false);
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
    return (SettingsActivity) requireActivity();
  }
}
