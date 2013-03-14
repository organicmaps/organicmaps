package com.mapswithme.maps.bookmarks.data;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.graphics.Point;

import com.mapswithme.util.Utils;

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

  public Bookmark getBookmark(ParcelablePointD p)
  {
    Point bookmark = getBookmark(p.x, p.y);
    if (bookmark.x == -1 && bookmark.y == -1)
    {
      final int index = getCategoriesCount() - 1;
      return new Bookmark(mContext, p, index, index >= 0 ? getCategoryById(index).getSize() : 0);
    }
    else
    {
      return new Bookmark(mContext, new BookmarkCategory(mContext, bookmark.x).getId(), bookmark.y);
    }
  }

  public ParcelablePoint findBookmark(ParcelablePointD p)
  {
    Point bookmark = getBookmark(p.x, p.y);
    if (bookmark.x >= 0 && bookmark.y >= 0)
      return new ParcelablePoint(bookmark);
    else
      return null;
  }

  public static native Point getBookmark(double px, double py);

  public Bookmark getBookmark(int cat, int bmk)
  {
    return new Bookmark(mContext, cat, bmk);
  }

  private String getUniqueName(String newName)
  {
    String name = newName;

    /// @todo Probably adding "-copy" suffix is better here (Mac OS style).
    int i = 0;
    while (isCategoryExist(name))
      name = newName + " " + (++i);

    return name;
  }

  public int createCategory(Bookmark bookmark, String newName)
  {
    bookmark.setCategory(getUniqueName(newName), getCategoriesCount());
    return getCategoriesCount() - 1;
  }

  public void setCategoryName(BookmarkCategory cat, String newName)
  {
    cat.setName(getUniqueName(newName));
  }

  private native boolean isCategoryExist(String name);

  /*
  public Bookmark previewBookmark(AddressInfo info)
  {
    return new Bookmark(mContext, info.getPosition(), info.getBookmarkName(mContext));
  }
   */

  public native void showBookmarkOnMap(int c, int b);

  private native String getNameForPlace(double px, double py);

  public String getNameForPlace(ParcelablePointD p)
  {
    return Utils.toTitleCase(getNameForPlace(p.x,p.y));
  }

  public AddressInfo getPOI(ParcelablePointD px)
  {
    return getPOI(px.x, px.y);
  }
  private native AddressInfo getPOI(double px, double py);

  public AddressInfo getAddressInfo(ParcelablePointD px)
  {
    return getAddressInfo(px.x, px.y);
  }
  private native AddressInfo getAddressInfo(double px, double py);

  public native String saveToKMZFile(int catID, String tmpPath);
}
