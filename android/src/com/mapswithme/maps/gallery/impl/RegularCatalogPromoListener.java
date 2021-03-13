package com.mapswithme.maps.gallery.impl;

import android.app.Activity;
import androidx.annotation.NonNull;
import android.text.TextUtils;

import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.promo.PromoEntity;
import com.mapswithme.util.UTM;

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
    if (TextUtils.isEmpty(item.getUrl()))
      return;

    String utmContentUrl = BookmarkManager.INSTANCE.injectCatalogUTMContent(item.getUrl(),
                                                                            UTM.UTM_CONTENT_DETAILS);
    BookmarksCatalogActivity.startForResult(mActivity, BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                            utmContentUrl);
  }

  @Override
  public void onMoreItemSelected(@NonNull PromoEntity item)
  {

    if (TextUtils.isEmpty(item.getUrl()))
      return;

    String utmContentUrl = BookmarkManager.INSTANCE.injectCatalogUTMContent(item.getUrl(),
                                                                            UTM.UTM_CONTENT_MORE);
    BookmarksCatalogActivity.startForResult(mActivity,
                                            BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                            utmContentUrl);
  }

  @Override
  public void onActionButtonSelected(@NonNull PromoEntity item, int position)
  {
    // Method not called.
  }
}
