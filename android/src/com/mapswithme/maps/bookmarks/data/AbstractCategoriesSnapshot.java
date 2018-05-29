package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class AbstractCategoriesSnapshot
{
  @NonNull
  private final List<BookmarkCategory> mSnapshot;

  public AbstractCategoriesSnapshot(@NonNull BookmarkCategory[] items)
  {
    mSnapshot = Collections.unmodifiableList(Arrays.asList(items));
  }

  @NonNull
  protected List<BookmarkCategory> items()
  {
    return mSnapshot;
  }

  public static class Default extends AbstractCategoriesSnapshot
  {
    @NonNull
    private final FilterStrategy mStrategy;

    protected Default(@NonNull BookmarkCategory[] items, @NonNull FilterStrategy strategy)
    {
      super(items);
      mStrategy = strategy;
    }

    @Override
    @NonNull
    public final List<BookmarkCategory> items()
    {
      return mStrategy.filter(super.items());
    }

    public int indexOfOrThrow(@NonNull BookmarkCategory category)
    {
      int indexOf = items().indexOf(category);
      if (indexOf < 0)
      {
        throw new UnsupportedOperationException(new StringBuilder("this category absent ")
                                                    .append("in current snapshot")
                                                    .append(indexOf)
                                                    .toString());
      }
      return indexOf;
    }

    public static Default from(BookmarkCategory[] bookmarkCategories,
                               FilterStrategy strategy)
    {
      return new Default(bookmarkCategories, strategy);
    }
  }

  public static class Private extends Default
  {
    public Private(@NonNull BookmarkCategory[] items)
    {
      super(items, new FilterStrategy.Private());
    }
  }

  public static class Catalog extends Default
  {
    public Catalog(@NonNull BookmarkCategory[] items)
    {
      super(items, new FilterStrategy.Catalog());
    }
  }

  public static class All extends Default {

    public All(@NonNull BookmarkCategory[] items)
    {
      super(items, new FilterStrategy.All());
    }
  }
}
