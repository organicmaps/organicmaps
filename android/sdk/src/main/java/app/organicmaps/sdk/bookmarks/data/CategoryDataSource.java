package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.sdk.content.DataSource;

public class CategoryDataSource extends RecyclerView.AdapterDataObserver implements DataSource<BookmarkCategory>
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

  @Override
  public void onChanged()
  {
    super.onChanged();
    // Looked up by id rather than scanned out of getCategories(), which holds top-level lists only: a child list
    // would never be found there and the screen would keep the snapshot it was opened with.
    final long id = mCategory.getId();
    if (BookmarkManager.INSTANCE.hasCategory(id))
      mCategory = BookmarkManager.INSTANCE.getCategoryById(id);
  }

  @Override
  public void invalidate()
  {
    onChanged();
  }
}
