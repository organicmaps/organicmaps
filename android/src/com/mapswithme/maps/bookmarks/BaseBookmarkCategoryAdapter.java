package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public abstract class BaseBookmarkCategoryAdapter<V extends RecyclerView.ViewHolder> extends RecyclerView.Adapter<V>
{
  private final Context mContext;

  public BaseBookmarkCategoryAdapter(Context context)
  {
    mContext = context;
  }

  protected Context getContext()
  {
    return mContext;
  }

  @Override
  public int getItemCount()
  {
    return BookmarkManager.INSTANCE.getCategoriesCount();
  }

  public BookmarkCategory getItem(int position)
  {
    return BookmarkManager.INSTANCE.getCategoryById(position);
  }
}
