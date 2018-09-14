package com.mapswithme.maps.gallery.impl;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.mapswithme.maps.discovery.ItemType;
import com.mapswithme.maps.gallery.Items;

public abstract class LoggableItemSelectedListener<I extends Items.Item> extends BaseItemSelectedListener<I>
{
  public LoggableItemSelectedListener(@NonNull Activity context, @NonNull ItemType type)
  {
    super(context, type);
  }

  @Override
  public final void onMoreItemSelected(@NonNull I item)
  {
    openUrl(item);
    onMoreItemSelectedInternal(item);
    getType().getMoreClickEvent().log();
  }

  @Override
  public final void onItemSelected(@NonNull I item, int position)
  {
    openUrl(item);
    onItemSelectedInternal(item, position);
    getType().getItemClickEvent().log();
  }

  protected abstract void onMoreItemSelectedInternal(@NonNull I item);

  protected abstract void onItemSelectedInternal(@NonNull I item, int position);
}
