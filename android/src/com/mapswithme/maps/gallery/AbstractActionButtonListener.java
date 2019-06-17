package com.mapswithme.maps.gallery;

import android.app.Activity;
import android.support.annotation.NonNull;

public abstract class AbstractActionButtonListener<T extends Items.Item> implements ItemSelectedListener<T>
{
  @NonNull
  private final Activity mActivity;

  public AbstractActionButtonListener(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  @Override
  public void onMoreItemSelected(@NonNull T item)
  {
  }

  @Override
  public void onActionButtonSelected(@NonNull T item, int position)
  {
  }

  @NonNull
  protected Activity getActivity()
  {
    return mActivity;
  }
}
