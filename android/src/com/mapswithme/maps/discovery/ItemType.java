package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.util.UiUtils;

public enum ItemType
{
  VIATOR,

  ATTRACTIONS(R.string.tourism,
              DiscoveryUserEvent.MORE_ATTRACTIONS_CLICKED,
              DiscoveryUserEvent.ATTRACTIONS_CLICKED)
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

  CAFES(R.string.eat, DiscoveryUserEvent.MORE_CAFES_CLICKED, DiscoveryUserEvent.CAFES_CLICKED)
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

  HOTELS(UiUtils.NO_ID, DiscoveryUserEvent.MORE_HOTELS_CLICKED,
         DiscoveryUserEvent.HOTELS_CLICKED)
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

  LOCAL_EXPERTS(UiUtils.NO_ID, DiscoveryUserEvent.MORE_LOCALS_CLICKED,
                DiscoveryUserEvent.LOCALS_CLICKED)
      {
        @Override
        public int getSearchCategory()
        {
          throw new UnsupportedOperationException("Unsupported.");
        }
      };

  @StringRes
  private final int mSearchCategory;
  @NonNull
  private final DiscoveryUserEvent mMoreClickEvent;
  @NonNull
  private final DiscoveryUserEvent mItemClickEvent;

  ItemType(@StringRes int searchCategory, @NonNull DiscoveryUserEvent moreClickEvent,
           @NonNull DiscoveryUserEvent itemClickEvent)
  {
    mSearchCategory = searchCategory;
    mMoreClickEvent = moreClickEvent;
    mItemClickEvent = itemClickEvent;
  }

  ItemType()
  {
    this(UiUtils.NO_ID, DiscoveryUserEvent.STUB, DiscoveryUserEvent.STUB);
  }

  @NonNull
  public DiscoveryUserEvent getMoreClickEvent()
  {
    return mMoreClickEvent;
  }

  @NonNull
  public DiscoveryUserEvent getItemClickEvent()
  {
    return mItemClickEvent;
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
