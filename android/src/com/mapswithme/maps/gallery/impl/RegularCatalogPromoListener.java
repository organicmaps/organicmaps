package com.mapswithme.maps.gallery.impl;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.promo.PromoEntity;

public class RegularCatalogPromoListener implements ItemSelectedListener<PromoEntity>
{
  @NonNull
  private final Activity mActivity;

  public RegularCatalogPromoListener(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  @Override
  public void onItemSelected(@NonNull PromoEntity item, int position)
  {
    BookmarksCatalogActivity.startByGuidesPageDeeplink(mActivity, item.getUrl());
  }

  @Override
  public void onMoreItemSelected(@NonNull PromoEntity item)
  {
    BookmarksCatalogActivity.startByGuidesPageDeeplink(mActivity, item.getUrl());
  }

  @Override
  public void onActionButtonSelected(@NonNull PromoEntity item, int position)
  {
    BookmarksCatalogActivity.startByGuidesPageDeeplink(mActivity, item.getUrl());
  }
}
