package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.NonNull;
import java.util.List;

class CacheBookmarkCategoriesDataProvider implements BookmarkCategoriesDataProvider
{
  @NonNull
  @Override
  public BookmarkCategory getCategoryById(long categoryId)
  {
    BookmarkManager.BookmarkCategoriesCache cache = BookmarkManager.INSTANCE.getBookmarkCategoriesCache();

    List<BookmarkCategory> categories = cache.getCategories();
    for (BookmarkCategory category : categories)
      if (category.getId() == categoryId)
        return category;

    return BookmarkManager.INSTANCE.nativeGetBookmarkCategory(categoryId);
  }

  @NonNull
  @Override
  public List<BookmarkCategory> getCategories()
  {
    return BookmarkManager.INSTANCE.getBookmarkCategoriesCache().getCategories();
  }

  @Override
  public int getCategoriesCount()
  {
    return BookmarkManager.INSTANCE.nativeGetBookmarkCategoriesCount();
  }
}
