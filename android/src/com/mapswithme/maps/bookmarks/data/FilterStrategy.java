package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import com.mapswithme.util.Predicate;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public interface FilterStrategy
{
  @NonNull
  List<BookmarkCategory> filter(@NonNull List<BookmarkCategory> items);

  class All implements FilterStrategy
  {
    @NonNull
    @Override
    public List<BookmarkCategory> filter(@NonNull List<BookmarkCategory> items)
    {
      return items;
    }
  }

  class Private extends PredicativeStrategy<Boolean>
  {
    public Private()
    {
      super(new Predicate.Equals<>(new BookmarkCategory.Downloaded(), false));
    }
  }

  class Downloaded extends PredicativeStrategy<Boolean>
  {
    Downloaded()
    {
      super(new Predicate.Equals<>(new BookmarkCategory.Downloaded(), true));
    }
  }

  class PredicativeStrategy<T> implements FilterStrategy
  {
    @NonNull
    private final Predicate<T, BookmarkCategory> mPredicate;

    private PredicativeStrategy(@NonNull Predicate<T, BookmarkCategory> predicate)
    {
      mPredicate = predicate;
    }

    @NonNull
    @Override
    public List<BookmarkCategory> filter(@NonNull List<BookmarkCategory> items)
    {
      List<BookmarkCategory> result = new ArrayList<>();
      for (BookmarkCategory each : items)
      {
        if (mPredicate.apply(each))
          result.add(each);
      }
      return Collections.unmodifiableList(result);
    }

    @NonNull
    public static FilterStrategy makeDownloadedInstance()
    {
      return new Downloaded();
    }

    @NonNull
    public static FilterStrategy makePrivateInstance()
    {
      return new Private();
    }
  }
}
