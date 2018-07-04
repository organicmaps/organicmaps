package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.text.TextUtils;

import com.mapswithme.maps.R;

public enum BookmarksPageFactory
{
  PRIVATE
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
  CATALOG(new AdapterResourceProvider.Catalog())
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
  private AdapterResourceProvider mResProvider;

  BookmarksPageFactory(@NonNull AdapterResourceProvider resourceProvider)
  {
    mResProvider = resourceProvider;
  }

  BookmarksPageFactory()
  {
    this(new AdapterResourceProvider.Default());
  }

  @NonNull
  public AdapterResourceProvider getResProvider()
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
    throw new IllegalArgumentException(new StringBuilder()
                                           .append("not found enum instance for value = ")
                                           .append(value)
                                           .toString());
  }

  @NonNull
  public abstract Fragment instantiateFragment();

  public abstract int getTitle();

}
