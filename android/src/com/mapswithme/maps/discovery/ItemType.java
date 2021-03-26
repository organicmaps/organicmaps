package com.mapswithme.maps.discovery;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.util.UiUtils;

public enum ItemType
{
  ATTRACTIONS(R.string.tourism)
      {
        @Override
        public void onResultReceived(@NonNull DiscoveryResultReceiver callback,
                                     @NonNull SearchResult[] results)
        {
          callback.onAttractionsReceived(results);
        }

        @Override
        public int getSearchCategory()
        {
          return getSearchCategoryInternal();
        }
      },

  CAFES(R.string.eat)
      {
        @Override
        public void onResultReceived(@NonNull DiscoveryResultReceiver callback,
                                     @NonNull SearchResult[] results)
        {
          callback.onCafesReceived(results);
        }

        @Override
        public int getSearchCategory()
        {
          return getSearchCategoryInternal();
        }
      },

  HOTELS(UiUtils.NO_ID)
      {
        @Override
        public int getSearchCategory()
        {
          throw new UnsupportedOperationException("Unsupported.");
        }
        @Override
        public void onResultReceived(@NonNull DiscoveryResultReceiver callback,
                                     @NonNull SearchResult[] results)
        {
          callback.onHotelsReceived(results);
        }
      },

  LOCAL_EXPERTS(UiUtils.NO_ID)
      {
        @Override
        public int getSearchCategory()
        {
          throw new UnsupportedOperationException("Unsupported.");
        }
      },

  PROMO(UiUtils.NO_ID);

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
