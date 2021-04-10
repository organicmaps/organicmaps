package com.mapswithme.maps.bookmarks;

import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.R;

public enum BookmarksPageFactory
{
  PRIVATE(true)
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

  DOWNLOADED(new BookmarkCategoriesPageResProvider.Catalog(),
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
  private final boolean mAdapterFooterAvailable;

  BookmarksPageFactory(@NonNull BookmarkCategoriesPageResProvider provider,
                       boolean hasAdapterFooter)
  {
    mResProvider = provider;
    mAdapterFooterAvailable = hasAdapterFooter;
  }

  BookmarksPageFactory(boolean hasAdapterFooter)
  {
    this(new BookmarkCategoriesPageResProvider.Default(), hasAdapterFooter);
  }

  @NonNull
  public BookmarkCategoriesPageResProvider getResProvider()
  {
    return mResProvider;
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
    throw new IllegalArgumentException("not found enum instance for value = " +
        value);
  }

  @NonNull
  protected abstract Fragment instantiateFragment();

  public abstract int getTitle();

  public boolean hasAdapterFooter()
  {
    return mAdapterFooterAvailable;
  }
}
