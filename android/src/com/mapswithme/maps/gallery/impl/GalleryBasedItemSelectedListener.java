package com.mapswithme.maps.gallery.impl;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.mapswithme.maps.discovery.ItemType;
import com.mapswithme.maps.gallery.Items;

public class GalleryBasedItemSelectedListener<I extends Items.Item> extends BaseItemSelectedListener<I>
{
  public GalleryBasedItemSelectedListener(@NonNull Activity context, @NonNull ItemType type)
  {
    super(context, type);
  }

  @Override
  public final void onMoreItemSelected(@NonNull I item)
  {
    onMoreItemSelectedInternal(item);
    getType().getMoreClickEvent().log();
  }

  @Override
  public final void onItemSelected(@NonNull I item, int position)
  {
    onItemSelectedInternal(item, position);
    getType().getItemClickEvent().log();
  }

  protected void onMoreItemSelectedInternal(@NonNull I item)
  {
    super.onMoreItemSelected(item);
  }

  protected void onItemSelectedInternal(@NonNull I item, int position)
  {
    super.onItemSelected(item, position);
  }
}
