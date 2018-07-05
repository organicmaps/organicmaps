package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.text.TextUtils;

import com.mapswithme.maps.R;

public enum BookmarksPageFactory
{
  PRIVATE(new Analytics(Constants.ANALYTICS_NAME_MY))
      {
        @NonNull
        @Override
        public Fragment instantiateFragment()
        {
          return new BookmarkCategoriesFragment();
        }

        @Override
        public int getTitle()
        {
          return R.string.bookmarks_page_my;
        }
      },
  CATALOG(new Analytics(Constants.ANALYTICS_NAME_DOWNLOADED), new AdapterResourceProvider.Catalog())
      {
        @NonNull
        @Override
        public Fragment instantiateFragment()
        {
          return new CachedBookmarkCategoriesFragment();
        }

        @Override
        public int getTitle()
        {
          return R.string.bookmarks_page_downloaded;
        }
      };

  @NonNull
  private final AdapterResourceProvider mResProvider;
  @NonNull
  private final Analytics mAnalytics;

  BookmarksPageFactory(@NonNull Analytics analytics, @NonNull AdapterResourceProvider provider)
  {
    mAnalytics = analytics;
    mResProvider = provider;
  }

  BookmarksPageFactory(Analytics analytics)
  {
    this(analytics, new AdapterResourceProvider.Default());
  }

  @NonNull
  public AdapterResourceProvider getResProvider()
  {
    return mResProvider;
  }

  @NonNull
  public Analytics getAnalytics()
  {
    return mAnalytics;
  }

  public static BookmarksPageFactory get(String value)
  {
    for (BookmarksPageFactory each : values())
    {
      if (TextUtils.equals(each.name(), value))
      {
        return each;
      }
    }
    throw new IllegalArgumentException(new StringBuilder()
                                           .append("not found enum instance for value = ")
                                           .append(value)
                                           .toString());
  }

  @NonNull
  public abstract Fragment instantiateFragment();

  public abstract int getTitle();

  public static class Analytics
  {
    @NonNull
    private final String mName;

    private Analytics(@NonNull String name)
    {
      mName = name;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }
  }

  private static class Constants
  {
    private static final String ANALYTICS_NAME_MY = "my";
    private static final String ANALYTICS_NAME_DOWNLOADED = "downloaded";
  }
}
