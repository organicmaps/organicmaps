package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;

public abstract class AbstractActionButtonCatalogPromoListener<T extends Items.Item> implements ItemSelectedListener<T>
{
  @NonNull
  private final Activity mActivity;

  public AbstractActionButtonCatalogPromoListener(@NonNull Activity activity)
  {
    mActivity = activity;
  }


  @Override
  public void onItemSelected(@NonNull T item, int position)
  {

  }

  @Override
  public void onMoreItemSelected(@NonNull T item)
  {

  }

  @NonNull
  protected Activity getActivity()
  {
    return mActivity;
  }
}
