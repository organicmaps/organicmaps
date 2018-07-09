package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.util.UiUtils;

public enum ItemType
{
  VIATOR,

  ATTRACTIONS(R.string.discovery_button_subtitle_attractions)
      {
        @Override
        public void onResultReceived(@NonNull DiscoveryResultReceiver callback, @NonNull SearchResult[] results)
        {
          callback.onAttractionsReceived(results);
        }

        @Override
        public int getSearchCategory()
        {
          return getSearchCategoryInternal();
        }
      },

  CAFES(R.string.food)
      {
        @Override
        public void onResultReceived(@NonNull DiscoveryResultReceiver callback, @NonNull SearchResult[] results)
        {
          callback.onCafesReceived(results);
        }

        @Override
        public int getSearchCategory()
        {
          return getSearchCategoryInternal();
        }
      },

  HOTELS
      {
        @Override
        public void onResultReceived(@NonNull DiscoveryResultReceiver callback, @NonNull SearchResult[] results)
        {
          callback.onHotelsReceived(results);
        }
      },

  LOCAL_EXPERTS;

  @StringRes
  private final int mSearchCategory;

  ItemType(@StringRes int searchCategory)
  {
    mSearchCategory = searchCategory;
  }

  ItemType()
  {
    this(UiUtils.NO_ID);
  }

  @StringRes
  protected int getSearchCategoryInternal()
  {
    return mSearchCategory;
  }

  @StringRes
  public int getSearchCategory()
  {
    throw new UnsupportedOperationException("Unsupported by default for " + name());
  }

  public void onResultReceived(@NonNull DiscoveryResultReceiver callback,
                               @NonNull SearchResult[] results)
  {
    /* Do nothing by default */
  }
}
