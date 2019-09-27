package com.mapswithme.maps.gallery.impl;

import android.app.Activity;
import androidx.annotation.NonNull;

import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.util.Utils;

public class BaseItemSelectedListener<I extends Items.Item>
    implements ItemSelectedListener<I>
{
  @NonNull
  private final Activity mContext;

  public BaseItemSelectedListener(@NonNull Activity context)
  {
    mContext = context;
  }

  @NonNull
  protected Activity getContext()
  {
    return mContext;
  }

  @Override
  public void onItemSelected(@NonNull I item, int position)
  {
    openUrl(item);
  }

  @Override
  public void onMoreItemSelected(@NonNull I item)
  {
    openUrl(item);
  }

  protected void openUrl(@NonNull I item)
  {
    Utils.openUrl(mContext, item.getUrl());
  }

  @Override
  public void onActionButtonSelected(@NonNull I item, int position)
  {
    openUrl(item);
  }
}
