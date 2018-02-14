package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public abstract class BaseBookmarkCategoryAdapter<V extends RecyclerView.ViewHolder> extends RecyclerView.Adapter<V>
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

  @Override
  public int getItemCount()
  {
    return BookmarkManager.INSTANCE.getCategoriesCount();
  }

  public long getItem(int position)
  {
    return BookmarkManager.INSTANCE.getCategoryIdByPosition(position);
  }
}
