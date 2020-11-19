package com.mapswithme.maps.bookmarks;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import android.text.TextUtils;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Analytics;
import com.mapswithme.util.statistics.Statistics;

public enum BookmarksPageFactory
{
  PRIVATE(new Analytics(Statistics.ParamValue.MY),
          true)
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

  DOWNLOADED(new Analytics(Statistics.ParamValue.DOWNLOADED),
             new BookmarkCategoriesPageResProvider.Catalog(),
             false)
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
  private final BookmarkCategoriesPageResProvider mResProvider;
  @NonNull
  private final Analytics mAnalytics;
  private final boolean mAdapterFooterAvailable;

  BookmarksPageFactory(@NonNull Analytics analytics,
                       @NonNull BookmarkCategoriesPageResProvider provider,
                       boolean hasAdapterFooter)
  {
    mAnalytics = analytics;
    mResProvider = provider;
    mAdapterFooterAvailable = hasAdapterFooter;
  }

  BookmarksPageFactory(Analytics analytics, boolean hasAdapterFooter)
  {
    this(analytics, new BookmarkCategoriesPageResProvider.Default(), hasAdapterFooter);
  }

  @NonNull
  public BookmarkCategoriesPageResProvider getResProvider()
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
  protected abstract Fragment instantiateFragment();

  public abstract int getTitle();

  public boolean hasAdapterFooter()
  {
    return mAdapterFooterAvailable;
  }
}
