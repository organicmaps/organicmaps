package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.NonNull;
import java.util.List;

public interface BookmarkCategoriesDataProvider
{
  @NonNull
  List<BookmarkCategory> getCategories();
  int getCategoriesCount();
  @NonNull
  BookmarkCategory getCategoryById(long categoryId);
}
