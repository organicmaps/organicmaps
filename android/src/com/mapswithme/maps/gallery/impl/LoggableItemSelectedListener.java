package com.mapswithme.maps.gallery.impl;

import android.app.Activity;
import androidx.annotation.NonNull;

import com.mapswithme.maps.discovery.ItemType;
import com.mapswithme.maps.gallery.Items;

public abstract class LoggableItemSelectedListener<I extends Items.Item> extends BaseItemSelectedListener<I>
{
  @NonNull
  private final ItemType mType;

  public LoggableItemSelectedListener(@NonNull Activity context, @NonNull ItemType type)
  {
    super(context);
    mType = type;
  }

  @Override
  public final void onMoreItemSelected(@NonNull I item)
  {
    super.onMoreItemSelected(item);
    onMoreItemSelectedInternal(item);
    mType.getMoreClickEvent().log();
  }

  @Override
  public final void onItemSelected(@NonNull I item, int position)
  {
    super.onItemSelected(item, position);
    onItemSelectedInternal(item, position);
    mType.getItemClickEvent().log();
  }

  @NonNull
  protected ItemType getType()
  {
    return mType;
  }

  protected abstract void onMoreItemSelectedInternal(@NonNull I item);

  protected abstract void onItemSelectedInternal(@NonNull I item, int position);
}
