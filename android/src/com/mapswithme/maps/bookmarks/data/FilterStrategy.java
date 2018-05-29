package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import com.mapswithme.maps.content.Predicate;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public abstract class FilterStrategy
{
  @NonNull
  public abstract List<BookmarkCategory> filter(@NonNull List<BookmarkCategory> items);

  public static class All extends FilterStrategy
  {
    @NonNull
    @Override
    public List<BookmarkCategory> filter(@NonNull List<BookmarkCategory> items)
    {
      return items;
    }
  }

  public static class Private extends PredicativeStrategy<Boolean>
  {
    public Private()
    {
      super(new Predicate.Equals<>(new BookmarkCategory.IsFromCatalog(), false));
    }

    public static FilterStrategy makeInstance()
    {
      return new Private();
    }
  }

  public static class Catalog extends PredicativeStrategy<Boolean>
  {
    Catalog()
    {
      super(new Predicate.Equals<>(new BookmarkCategory.IsFromCatalog(), true));
    }

    public static Catalog makeInstance()
    {
      return new Catalog();
    }
  }

  public static class PredicativeStrategy<T> extends FilterStrategy
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
        {
          result.add(each);
        }
      }
      return Collections.unmodifiableList(result);
    }
  }
}
