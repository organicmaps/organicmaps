package com.mapswithme.maps.bookmarks.data;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.mapswithme.maps.R;
import com.mapswithme.maps.R.drawable;

import android.content.Context;
import android.graphics.BitmapFactory;
import android.graphics.Point;

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

  public static BookmarkManager getPinManager(Context context)
  {
    if (sManager == null)
    {
      sManager = new BookmarkManager(context.getApplicationContext());
    }

    return sManager;
  }

  public List<BookmarkCategory> getPinSets()
  {

    return mPinSets;
  }

  public int getPinId(Bookmark pin)
  {
    return mPins.indexOf(pin);
  }
/*
  public Bookmark createNewBookmark()
  {
    Bookmark p;
   // mPins.add(p = new Bookmark("", mIcons.get(R.drawable.placemark_red)));
    return p;
  }*/

  private void refreshList()
  {
    for (int i = 0; i < 10; i++)
    {
      putBookmark(50*i, 25*i, "name " + i, "category "+i);
    }
    /*
    mPins = new ArrayList<Bookmark>();
    mPinSets = new ArrayList<BookmarkCategory>();
    mIcons = loadIcons();
    for (int i = 0; i < 25; i++)
    {
      BookmarkCategory set = new BookmarkCategory("Set " + (i + 1));
      mPinSets.add(set);
      for (Icon icon : mIcons.values())
      {
        Bookmark p = new Bookmark(set.getName() + " pin #" + icon.getDrawableId(), icon);
        p.setPinSet(set);
        mPins.add(p);
      }
    }*/
  }


  /// All method below it implemented in jni

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
    return new BookmarkCategory(mContext, id);
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
    if (bookmark == null)
    {
      return new Bookmark(mContext, p);
    }
    else
    {
      return new Bookmark(mContext, new BookmarkCategory(mContext, bookmark[0]).getId(), bookmark[1]);
    }
  }

  private native int[] nGetBookmark(int x, int y);

  public Bookmark getBookmark(int cat, int bmk)
  {

    return new Bookmark(mContext, cat, bmk);
  }
}
