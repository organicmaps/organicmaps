package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import android.content.Context;
import android.widget.BaseAdapter;

public abstract class AbstractBookmarkCategoryAdapter extends BaseAdapter
{
  private BookmarkManager mManager;
  private Context mContext;

  public AbstractBookmarkCategoryAdapter(Context context)
  {
    mContext = context;
    mManager = BookmarkManager.getPinManager(context);
  }

  protected Context getContext()
  {
    return mContext;
  }

  protected BookmarkManager getBookmarkManager()
  {
    return mManager;
  }

  @Override
  public int getCount()
  {
    return mManager.getCategoriesCount();
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public BookmarkCategory getItem(int position)
  {
    return getBookmarkManager().getCategoryById(position);
  }
}
