package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.content.DataSource;

public class CategoryDataSource extends RecyclerView.AdapterDataObserver implements
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

  @Override
  public void onChanged()
  {
    super.onChanged();
    AbstractCategoriesSnapshot.Default snapshot =
        BookmarkManager.INSTANCE.getCategoriesSnapshot(mCategory.getType().getFilterStrategy());

    int index = snapshot.indexOfOrThrow(mCategory);
    mCategory = snapshot.getItems().get(index);
  }
}
