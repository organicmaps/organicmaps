package com.mapswithme.maps.tips;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.maplayer.Mode;
import com.mapswithme.maps.search.FilterUtils;
import com.mapswithme.util.UTM;

class ClickInterceptorFactory
{
  @NonNull
  static ClickInterceptor createActivateIsolinesLayerListener()
  {
    return new ActivateIsolinesLayer();
  }

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
      super(Tutorial.BOOKMARKS);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      String catalogUrl = BookmarkManager.INSTANCE.getCatalogFrontendUrl(UTM.UTM_TIPS_AND_TRICKS);
      BookmarksCatalogActivity.startForResult(activity,
                                              BookmarkCategoriesActivity
                                                  .REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY, catalogUrl);
    }
  }

  static class ActivateIsolinesLayer extends AbstractClickInterceptor
  {
    ActivateIsolinesLayer()
    {
      super(Tutorial.ISOLINES);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      Mode.ISOLINES.setEnabled(activity, true);
      activity.onIsolinesLayerSelected();
    }
  }

  static class ActivateSubwayLayer extends AbstractClickInterceptor
  {

    ActivateSubwayLayer()
    {
      super(Tutorial.SUBWAY);
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
      super(Tutorial.SEARCH);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      activity.showSearch(FilterUtils.getHotelCategoryString(activity));
    }
  }

  static class OpenDiscoveryScreen extends AbstractClickInterceptor
  {
    OpenDiscoveryScreen()
    {
      super(Tutorial.DISCOVERY);
    }

    @Override
    public void onInterceptClickInternal(@NonNull MwmActivity activity)
    {
      activity.showDiscovery();
    }
  }
}
