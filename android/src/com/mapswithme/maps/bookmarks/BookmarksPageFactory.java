package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.text.TextUtils;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Analytics;
import com.mapswithme.util.statistics.Statistics;

public enum BookmarksPageFactory
{
  PRIVATE(new Analytics(Statistics.ParamValue.MY))
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
          return R.string.bookmarks;
        }
      },
  CATALOG(new Analytics(Statistics.ParamValue.DOWNLOADED), new AdapterResourceProvider.Catalog())
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
          return R.string.guides;
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

}
