package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import java.util.List;

public abstract class BaseBookmarkCategoryAdapter<V extends RecyclerView.ViewHolder>
    extends RecyclerView.Adapter<V>
{
  @NonNull
  private final Context mContext;

  BaseBookmarkCategoryAdapter(@NonNull Context context)
  {
    mContext = context;
  }

  @NonNull
  protected Context getContext()
  {
    return mContext;
  }

  @NonNull
  public List<BookmarkCategory> getBookmarkCategories()
  {
    return BookmarkManager.INSTANCE.getOwnedCategoriesSnapshot().items();
  }

  @Override
  public int getItemCount()
  {
    return getBookmarkCategories().size();
  }

  @NonNull
  public BookmarkCategory getCategoryByPosition(int position)
  {
    List<BookmarkCategory> categories = getBookmarkCategories();
    if (position < 0 || position > categories.size() - 1)
      throw new ArrayIndexOutOfBoundsException(position);
    return categories.get(position);

  }
}
