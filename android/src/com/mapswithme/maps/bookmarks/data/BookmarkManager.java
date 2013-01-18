package com.mapswithme.maps.bookmarks.data;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.graphics.Point;

import com.mapswithme.maps.R;

public class BookmarkManager
{
  private static BookmarkManager sManager;
  private List<Bookmark> mPins;
  private List<BookmarkCategory> mPinSets;
  private Context mContext;
  private BookmarkIconManager mIconManager;

  private BookmarkManager(Context context)
  {
    mContext = context;
    refreshList();
    mIconManager = new BookmarkIconManager(context);
  }

  public static BookmarkManager getBookmarkManager(Context context)
  {
    if (sManager == null)
    {
      sManager = new BookmarkManager(context.getApplicationContext());
    }

    return sManager;
  }

  private void refreshList()
  {
    nLoadBookmarks();
  }

  private native void nLoadBookmarks();

  public void deleteBookmark(int cat, int bmk)
  {
    nDeleteBookmark(cat, bmk);
  }

  private native void putBookmark(int x, int y, String bokmarkName, String categoryName);
  private native void nDeleteBookmark(int x, int y);

  public void addBookmark(int x, int y, String bokmarkName, String categoryName)
  {
    putBookmark(x, y, bokmarkName, categoryName);
  }

  public BookmarkCategory getCategoryById(int id)
  {
    if (id < getCategoriesCount())
    {
      return new BookmarkCategory(mContext, id);
    }
    else
    {
      return null;
    }
  }

  public native int getCategoriesCount();

  public void deleteCategory(int index)
  {
    nDeleteCategory(index);
  }

  private native boolean nDeleteCategory(int index);

  Icon getIconByName(String name)
  {
    return mIconManager.getIcon(name);
  }

  public List<Icon> getIcons()
  {
    return new ArrayList<Icon>(mIconManager.getAll().values());
  }

  public Bookmark getBookmark(Point p)
  {
    int [] bookmark = nGetBookmark(p.x, p.y);
    if (bookmark[0] == -1 && bookmark[1] == -1)
    {
      return new Bookmark(mContext, p, getCategoriesCount() - 1, 0);
    }
    else
    {
      return new Bookmark(mContext, new BookmarkCategory(mContext, bookmark[0]).getId(), bookmark[1]);
    }
  }

  public ParcelablePoint findBookmark(Point p)
  {
    int [] bookmark = nGetBookmark(p.x, p.y);
    if (bookmark[0]>=0 && bookmark[1]>=0)
    {
      return new ParcelablePoint(bookmark[0], bookmark[1]);
    }
    else
      return null;
  }

  private native int[] nGetBookmark(int x, int y);

  public Bookmark getBookmark(int cat, int bmk)
  {

    return new Bookmark(mContext, cat, bmk);
  }

  public BookmarkCategory createCategory(Bookmark bookmark)
  {
    String pattern;
    String name = pattern = mContext.getResources().getString(R.string.new_places);
    int i = 0;
    while (getCategoryByName(name))
    {
      name = pattern + " " + (++i);
    }
    bookmark.setCategory(name, getCategoriesCount());
    BookmarkCategory cat = new BookmarkCategory(mContext, getCategoriesCount()-1);
    return cat;
  }

  //TODO
  public boolean getCategoryByName(String name)
  {
    return nGetCategoryByName(name);
  }

  private native boolean nGetCategoryByName(String name);

  public Bookmark previewBookmark(Point point)
  {
    return new Bookmark(mContext, point);
  }
}
