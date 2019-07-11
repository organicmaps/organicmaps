package com.mapswithme.maps.gallery.impl;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.text.TextUtils;

import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.promo.PromoEntity;
import com.mapswithme.util.statistics.Destination;
import com.mapswithme.util.statistics.GalleryPlacement;
import com.mapswithme.util.statistics.GalleryType;
import com.mapswithme.util.statistics.Statistics;

public class RegularCatalogPromoListener implements ItemSelectedListener<PromoEntity>
{
  @NonNull
  private final Activity mActivity;
  @NonNull
  private final GalleryPlacement mPlacement;

  public RegularCatalogPromoListener(@NonNull Activity activity, @NonNull GalleryPlacement placement)
  {
    mActivity = activity;
    mPlacement = placement;
  }

  @Override
  public void onItemSelected(@NonNull PromoEntity item, int position)
  {
    if (TextUtils.isEmpty(item.getUrl()))
      return;

    BookmarksCatalogActivity.startByGuidesPageDeeplink(mActivity, item.getUrl());
    Statistics.INSTANCE.trackGalleryProductItemSelected(GalleryType.PROMO, mPlacement, position,
                                                        Destination.CATALOGUE);
  }

  @Override
  public void onMoreItemSelected(@NonNull PromoEntity item)
  {

    if (TextUtils.isEmpty(item.getUrl()))
      return;

    BookmarksCatalogActivity.startByGuidesPageDeeplink(mActivity, item.getUrl());
    Statistics.INSTANCE.trackGalleryEvent(Statistics.EventName.PP_SPONSOR_MORE_SELECTED,
                                          GalleryType.PROMO,
                                          mPlacement);
  }

  @Override
  public void onActionButtonSelected(@NonNull PromoEntity item, int position)
  {
    if (TextUtils.isEmpty(item.getUrl()))
      return;

    BookmarksCatalogActivity.startByGuidesPageDeeplink(mActivity, item.getUrl());
  }
}
