package com.mapswithme.maps.bookmarks.data;

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.util.Statistics;

import android.content.Context;
import android.graphics.Point;
import android.util.Pair;

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
    loadBookmarks();
  }

  private native void loadBookmarks();

  public void deleteBookmark(Bookmark bmk)
  {
    deleteBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
  }

  public int addBookmarkToLastEditedCategory(String name, double lat, double lon)
  {
    Statistics.INSTANCE.trackBookmarkCreated(mContext);
    return nativeAddBookmarkToLastEditedCategory(name, lat, lon);
  }

  public int getLastEditedCategory()
  {
    return nativeGetLastEditedCategory();
  }

  public native void deleteBookmark(int c, int b);

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

  public native boolean deleteCategory(int index);

  public Icon getIconByName(String name)
  {
    return mIconManager.getIcon(name);
  }

  public List<Icon> getIcons()
  {
    return new ArrayList<Icon>(mIconManager.getAll().values());
  }

  public static native Point getBookmark(double px, double py);

  public Bookmark getBookmark(int cat, int bmk)
  {
    return new Bookmark(mContext, cat, bmk);
  }

  private native int createCategory(String name);

  public int createCategory(Bookmark bookmark, String newName)
  {
    final int cat = createCategory(newName);
    bookmark.setCategoryId(cat);
    return cat;
  }

  public void setCategoryName(BookmarkCategory cat, String newName)
  {
    cat.setName(newName);
  }

  public Pair<Integer, Integer> addNewBookmark(String name, double lat, double lon)
  {
    final int cat = getLastEditedCategory();
    final int bmk = addBookmarkToLastEditedCategory(name, lat, lon);

    return new Pair<Integer, Integer>(cat, bmk);
  }

  public native void showBookmarkOnMap(int c, int b);

  public native String saveToKMZFile(int catID, String tmpPath);

  public native int nativeAddBookmarkToLastEditedCategory(String name, double lat, double lon);

  public native int nativeGetLastEditedCategory();
}
