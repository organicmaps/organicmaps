package com.mapswithme.maps.bookmarks.data;

import android.util.Pair;

import java.util.Arrays;
import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

public enum BookmarkManager
{
  INSTANCE;

  private static final Icon[] ICONS = {
      new Icon("placemark-red", "placemark-red", R.drawable.ic_bookmark_marker_red_off, R.drawable.ic_bookmark_marker_red_on),
      new Icon("placemark-blue", "placemark-blue", R.drawable.ic_bookmark_marker_blue_off, R.drawable.ic_bookmark_marker_blue_on),
      new Icon("placemark-purple", "placemark-purple", R.drawable.ic_bookmark_marker_purple_off, R.drawable.ic_bookmark_marker_purple_on),
      new Icon("placemark-yellow", "placemark-yellow", R.drawable.ic_bookmark_marker_yellow_off, R.drawable.ic_bookmark_marker_yellow_on),
      new Icon("placemark-pink", "placemark-pink", R.drawable.ic_bookmark_marker_pink_off, R.drawable.ic_bookmark_marker_pink_on),
      new Icon("placemark-brown", "placemark-brown", R.drawable.ic_bookmark_marker_brown_off, R.drawable.ic_bookmark_marker_brown_on),
      new Icon("placemark-green", "placemark-green", R.drawable.ic_bookmark_marker_green_off, R.drawable.ic_bookmark_marker_green_on),
      new Icon("placemark-orange", "placemark-orange", R.drawable.ic_bookmark_marker_orange_off, R.drawable.ic_bookmark_marker_orange_on)
  };

  BookmarkManager()
  {
    loadBookmarks();
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

  private native void deleteBookmark(int c, int b);

  public BookmarkCategory getCategoryById(int id)
  {
    if (id < getCategoriesCount())
      return new BookmarkCategory(id);
    else
      return null;
  }

  public native int getCategoriesCount();

  public native boolean deleteCategory(int index);

  public static Icon getIconByType(String type)
  {
    for (Icon icon : ICONS)
    {
      if (icon.getType().equals(type))
        return icon;
    }
    // return default icon
    return ICONS[0];
  }

  public void toggleCategoryVisibility(int index)
  {
    BookmarkCategory category = getCategoryById(index);
    if (category != null)
      category.setVisibility(!category.isVisible());
  }

  public static List<Icon> getIcons()
  {
    return Arrays.asList(ICONS);
  }

  public Bookmark getBookmark(Pair<Integer, Integer> catAndBmk)
  {
    return getBookmark(catAndBmk.first, catAndBmk.second);
  }

  public Bookmark getBookmark(int cat, int bmk)
  {
    return getCategoryById(cat).getBookmark(bmk);
  }

  public Pair<Integer, Integer> addNewBookmark(String name, double lat, double lon)
  {
    final int cat = getLastEditedCategory();
    final int bmk = addBookmarkToLastEditedCategory(name, lat, lon);
    Statistics.INSTANCE.trackBookmarkCreated();

    return new Pair<>(cat, bmk);
  }

  public native int createCategory(String name);

  public native void showBookmarkOnMap(int c, int b);

  public native String saveToKmzFile(int catId, String tmpPath);

  public native int addBookmarkToLastEditedCategory(String name, double lat, double lon);

  public native int getLastEditedCategory();

  public static native String generateUniqueBookmarkName(String baseName);

  public static native boolean loadKmzFile(String path);
}
