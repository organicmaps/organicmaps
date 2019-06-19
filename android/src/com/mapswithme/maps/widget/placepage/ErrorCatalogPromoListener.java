package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.mapswithme.maps.gallery.Items;
import com.mapswithme.util.Utils;

public class ErrorCatalogPromoListener<T extends Items.Item> implements com.mapswithme.maps.gallery.ItemSelectedListener<T>
{
  @NonNull
  private final Activity mActivity;

  public ErrorCatalogPromoListener(@NonNull Activity activity)
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

  @Override
  public void onItemSelected(@NonNull T item, int position)
  {
    Utils.showSystemSettings(getActivity());
  }

  @NonNull
  protected Activity getActivity()
  {
    return mActivity;
  }
}
