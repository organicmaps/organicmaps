package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.widget.BaseAdapter;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public abstract class AbstractBookmarkCategoryAdapter extends BaseAdapter
{
  private final Context mContext;

  public AbstractBookmarkCategoryAdapter(Context context)
  {
    mContext = context;
  }

  protected Context getContext()
  {
    return mContext;
  }

  @Override
  public int getCount()
  {
    return BookmarkManager.INSTANCE.getCategoriesCount();
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public BookmarkCategory getItem(int position)
  {
    return BookmarkManager.INSTANCE.getCategoryById(position);
  }
}
