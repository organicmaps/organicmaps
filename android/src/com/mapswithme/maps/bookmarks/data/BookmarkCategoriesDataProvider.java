package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.support.annotation.NonNull;

import com.mapswithme.maps.BookmarkCategoriesCache;

import java.util.Arrays;
import java.util.List;

public interface BookmarkCategoriesDataProvider
{
  @NonNull
  List<BookmarkCategory> getCategories();

  @NonNull
  BookmarkCategory getCategoryById(long categoryId);

  public abstract class AbstractDataProvider implements BookmarkCategoriesDataProvider
  {

    @NonNull
    public AbstractCategoriesSnapshot.Default getAllCategoriesSnapshot()
    {
      List<BookmarkCategory> items = getCategories();
      return new AbstractCategoriesSnapshot.Default(items, new FilterStrategy.All());
    }
  }

  public static class CoreBookmarkCategoriesDataProvider extends AbstractDataProvider
  {

    @NonNull
    @Override
    public List<BookmarkCategory> getCategories()
    {
      BookmarkCategory[] categories = BookmarkManager.INSTANCE.nativeGetBookmarkCategories();
      return Arrays.asList(categories);
    }

    @NonNull
    @Override
    public BookmarkCategory getCategoryById(long categoryId)
    {
      BookmarkCategory[] categories =
          BookmarkManager.INSTANCE.nativeGetBookmarkCategories();

      for (BookmarkCategory each : categories)
      {
        if (each.getId() == categoryId)
          return each;

      }
      throw new IllegalArgumentException("There is no category for id : " + categoryId);
    }
  }

  public static class CacheBookmarkCategoriesDataProvider extends AbstractDataProvider
  {
    @NonNull
    private final Context mContext;
    @NonNull
    private final BookmarkCategoriesDataProvider mCoreDataProvider;

    public CacheBookmarkCategoriesDataProvider(@NonNull Context context,
                                               @NonNull BookmarkCategoriesDataProvider dataProvider)
    {
      mContext = context;
      mCoreDataProvider = dataProvider;
    }

    @NonNull
    @Override
    public List<BookmarkCategory> getCategories()
    {
      return BookmarkCategoriesCache.from(mContext).getItems();
    }

    private void updateCache()
    {
      BookmarkCategoriesCache cache = BookmarkCategoriesCache.from(mContext);
      cache.updateItems(mCoreDataProvider.getCategories());
    }

    @NonNull
    public BookmarkCategory getCategoryById(long catId)
    {
      List<BookmarkCategory> items = getAllCategoriesSnapshot().getItems();
      for (BookmarkCategory each : items)
      {
        if (catId == each.getId())
          return each;
      }
      throw new IllegalArgumentException(new StringBuilder().append("Category with id = ")
                                                            .append(catId)
                                                            .append(" missed")
                                                            .toString());
    }
  }
}
