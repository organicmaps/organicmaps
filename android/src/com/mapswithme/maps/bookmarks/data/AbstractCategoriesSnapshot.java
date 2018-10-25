package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public abstract class AbstractCategoriesSnapshot
{
  @NonNull
  private final List<BookmarkCategory> mSnapshot;

  AbstractCategoriesSnapshot(@NonNull BookmarkCategory[] items)
  {
    mSnapshot = Collections.unmodifiableList(Arrays.asList(items));
  }

  @NonNull
  protected List<BookmarkCategory> getItems()
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
    public final List<BookmarkCategory> getItems()
    {
      return mStrategy.filter(super.getItems());
    }

    public int indexOfOrThrow(@NonNull BookmarkCategory category)
    {
      return indexOfThrowInternal(getItems(), category);
    }

    private static int indexOfThrowInternal(@NonNull List<BookmarkCategory> categories,
                                            @NonNull BookmarkCategory category)
    {
      int indexOf = categories.indexOf(category);
      if (indexOf < 0)
      {
        throw new UnsupportedOperationException(new StringBuilder("This category absent in " +
                                                                  "current snapshot ")
                                                    .append(category)
                                                    .append("all items : ")
                                                    .append(Arrays.toString(categories.toArray()))
                                                    .toString());
      }
      return indexOf;
    }

    @NonNull
    public BookmarkCategory refresh(@NonNull BookmarkCategory category)
    {
      List<BookmarkCategory> items = getItems();
      int index = indexOfThrowInternal(items, category);
      return items.get(index);
    }
  }
}
