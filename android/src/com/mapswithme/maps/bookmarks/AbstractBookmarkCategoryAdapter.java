package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import android.content.Context;
import android.widget.BaseAdapter;

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
    return mManager.getCategoriesCount() + 1;
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  public final static int ITEM = 0;
  public final static int HELP = 1;
  @Override
  public int getItemViewType(int position)
  {
    if (position == getCount() - 1) return HELP;
    return ITEM;
  }

  @Override
  public int getViewTypeCount()
  {
    return 2;
  }

  public boolean isActiveItem(int position)
  {
    return getItemViewType(position) != HELP
        && position < getCount()
        && position >= 0;
  }

  @Override
  public BookmarkCategory getItem(int position)
  {
    return getBookmarkManager().getCategoryById(position);
  }
}
