package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.widget.BaseAdapter;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public abstract class AbstractBookmarkCategoryAdapter extends BaseAdapter
{
  private final BookmarkManager mManager;
  private final Context mContext;

  public AbstractBookmarkCategoryAdapter(Context context)
  {
    mContext = context;
    mManager = BookmarkManager.getBookmarkManager(context);
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
