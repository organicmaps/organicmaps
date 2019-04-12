package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.widget.placepage.PlacePageButtons;

public enum RoadWarningMarkType
{
  TOLL(PlacePageButtons.Item.ROUTE_AVOID_TOLL),
  FERRY(PlacePageButtons.Item.ROUTE_AVOID_FERRY),
  DIRTY(PlacePageButtons.Item.ROUTE_AVOID_UNPAVED),
  UNKNOWN;

  @Nullable
  private final PlacePageButtons.Item mItem;

  RoadWarningMarkType(@Nullable PlacePageButtons.Item placePageBtnItem)
  {
    mItem = placePageBtnItem;
  }

  RoadWarningMarkType()
  {
    this(null);
  }

  @NonNull
  public PlacePageButtons.Item getPlacePageItem()
  {
    if (mItem == null)
      throw new UnsupportedOperationException("For " + name() +  " getItem() is forbidden");

    return mItem;
  }
}
