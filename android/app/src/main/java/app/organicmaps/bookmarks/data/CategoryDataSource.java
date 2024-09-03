package app.organicmaps.bookmarks.data;

import androidx.annotation.NonNull;

import app.organicmaps.content.DataSource;

import java.util.List;

public class CategoryDataSource implements
                                DataSource<BookmarkCategory>
{
  @NonNull
  private BookmarkCategory mCategory;

  public CategoryDataSource(@NonNull BookmarkCategory category)
  {
    mCategory = category;
  }

  @NonNull
  @Override
  public BookmarkCategory getData()
  {
    return mCategory;
  }

  public void invalidate()
  {
    List<BookmarkCategory> categories = BookmarkManager.INSTANCE.getCategories();
    int index = categories.indexOf(mCategory);
    if (index >= 0)
      mCategory = categories.get(index);
  }
}
