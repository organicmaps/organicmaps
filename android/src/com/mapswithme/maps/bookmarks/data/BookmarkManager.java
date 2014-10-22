package com.mapswithme.maps.bookmarks.data;

import android.util.Pair;

import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.List;

public class BookmarkManager
{
  private static BookmarkManager sManager;

  private BookmarkManager()
  {
    loadBookmarks();
  }

  public static BookmarkManager getBookmarkManager()
  {
    if (sManager == null)
      sManager = new BookmarkManager();
    return sManager;
  }

  private native void loadBookmarks();

  public void deleteBookmark(Bookmark bmk)
  {
    deleteBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
  }

  public void deleteTrack(Track track)
  {
    nativeDeleteTrack(track.getCategoryId(), track.getTrackId());
  }

  private native void nativeDeleteTrack(int cat, int trk);

  public int addBookmarkToLastEditedCategory(String name, double lat, double lon)
  {
    Statistics.INSTANCE.trackBookmarkCreated();
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
      return new BookmarkCategory(id);
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
    return BookmarkIconManager.getIcon(name);
  }

  public List<Icon> getIcons()
  {
    return new ArrayList<Icon>(BookmarkIconManager.getAll().values());
  }

  public Bookmark getBookmark(Pair<Integer, Integer> catAndBmk)
  {
    return getBookmark(catAndBmk.first, catAndBmk.second);
  }

  public Bookmark getBookmark(int cat, int bmk)
  {
    return getCategoryById(cat).getBookmark(bmk);
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
