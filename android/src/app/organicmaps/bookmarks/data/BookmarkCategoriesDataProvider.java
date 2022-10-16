package app.organicmaps.bookmarks.data;

import androidx.annotation.NonNull;

import java.util.List;

public interface BookmarkCategoriesDataProvider
{
  @NonNull
  List<BookmarkCategory> getCategories();
  @NonNull
  List<BookmarkCategory> getChildrenCategories(long parentId);
  @NonNull
  BookmarkCategory getCategoryById(long categoryId);
}
