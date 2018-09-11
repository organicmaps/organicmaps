package com.mapswithme.maps.tips;

import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.maplayer.Mode;

class ClickInterceptorFactory
{
  @NonNull
  static ClickInterceptor createActivateSubwayLayerListener()
  {
    return new ActivateSubwayLayer();
  }

  @NonNull
  static ClickInterceptor createOpenDiscoveryScreenListener()
  {
    return new OpenDiscoveryScreen();
  }

  @NonNull
  static ClickInterceptor createSearchHotelsListener()
  {
    return new SearchHotels();
  }

  @NonNull
  static ClickInterceptor createOpenBookmarksCatalogListener()
  {
    return new OpenBookmarksCatalog();
  }

  static class OpenBookmarksCatalog extends AbstractClickInterceptor
  {
    OpenBookmarksCatalog()
    {
      super(TipsApi.BOOKMARKS);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      BookmarksCatalogActivity.startForResult(activity,
                                              BookmarkCategoriesActivity
                                                  .REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY);
    }
  }

  static class ActivateSubwayLayer extends AbstractClickInterceptor
  {
    ActivateSubwayLayer()
    {
      super(TipsApi.MAP_LAYERS);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      Mode.SUBWAY.setEnabled(activity, true);
      activity.onSubwayLayerSelected();
    }
  }

  static class SearchHotels extends AbstractClickInterceptor
  {
    SearchHotels()
    {
      super(TipsApi.SEARCH);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      activity.showSearch(activity.getString(R.string.hotel));
    }
  }

  static class OpenDiscoveryScreen extends AbstractClickInterceptor
  {
    OpenDiscoveryScreen()
    {
      super(TipsApi.DISCOVERY);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      activity.showDiscovery();
    }
  }
}
