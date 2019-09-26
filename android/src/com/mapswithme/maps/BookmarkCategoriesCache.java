package com.mapswithme.maps;

import android.content.Context;
import android.database.Observable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class BookmarkCategoriesCache extends Observable<RecyclerView.AdapterDataObserver>
{
  @NonNull
  private List<BookmarkCategory> mCachedItems = new ArrayList<>();

  public void updateItems(@NonNull List<BookmarkCategory> cachedItems)
  {
    mCachedItems = Collections.unmodifiableList(cachedItems);
    notifyChanged();
  }

  @NonNull
  public List<BookmarkCategory> getItems()
  {
    return mCachedItems;
  }

  private void notifyChanged() {
    for (int i = mObservers.size() - 1; i >= 0; i--) {
      mObservers.get(i).onChanged();
    }
  }

  @NonNull
  public static BookmarkCategoriesCache from(@NonNull Context context)
  {
    return MwmApplication.from(context).getBookmarkCategoriesCache();
  }
}
