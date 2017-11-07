package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

@MainThread
public class BookmarkManager
{
  public static final BookmarkManager INSTANCE = new BookmarkManager();

  public static final List<Icon> ICONS = new ArrayList<>();

  private List<BookmarksLoadingListener> mListeners = new ArrayList<>();

  private BookmarkManager() {}

  static
  {
    ICONS.add(new Icon("placemark-red", "placemark-red", R.drawable.ic_bookmark_marker_red_off, R.drawable.ic_bookmark_marker_red_on));
    ICONS.add(new Icon("placemark-blue", "placemark-blue", R.drawable.ic_bookmark_marker_blue_off, R.drawable.ic_bookmark_marker_blue_on));
    ICONS.add(new Icon("placemark-purple", "placemark-purple", R.drawable.ic_bookmark_marker_purple_off, R.drawable.ic_bookmark_marker_purple_on));
    ICONS.add(new Icon("placemark-yellow", "placemark-yellow", R.drawable.ic_bookmark_marker_yellow_off, R.drawable.ic_bookmark_marker_yellow_on));
    ICONS.add(new Icon("placemark-pink", "placemark-pink", R.drawable.ic_bookmark_marker_pink_off, R.drawable.ic_bookmark_marker_pink_on));
    ICONS.add(new Icon("placemark-brown", "placemark-brown", R.drawable.ic_bookmark_marker_brown_off, R.drawable.ic_bookmark_marker_brown_on));
    ICONS.add(new Icon("placemark-green", "placemark-green", R.drawable.ic_bookmark_marker_green_off, R.drawable.ic_bookmark_marker_green_on));
    ICONS.add(new Icon("placemark-orange", "placemark-orange", R.drawable.ic_bookmark_marker_orange_off, R.drawable.ic_bookmark_marker_orange_on));
    ICONS.add(new Icon("placemark-hotel", "placemark-hotel", R.drawable.ic_bookmark_marker_hotel_off, R.drawable.ic_bookmark_marker_hotel_on));
  }

  static Icon getIconByType(String type)
  {
    for (Icon icon : ICONS)
    {
      if (icon.getType().equals(type))
        return icon;
    }
    // return default icon
    return ICONS.get(0);
  }

  public void deleteBookmark(Bookmark bmk)
  {
    nativeDeleteBookmark(bmk.getCategoryId(), bmk.getBookmarkId());
  }

  public void deleteTrack(Track track)
  {
    nativeDeleteTrack(track.getCategoryId(), track.getTrackId());
  }

  public @NonNull BookmarkCategory getCategory(int catId)
  {
    if (catId < nativeGetCategoriesCount())
      return new BookmarkCategory(catId);

    throw new IndexOutOfBoundsException("Invalid category ID!");
  }

  public void toggleCategoryVisibility(int catId)
  {
    BookmarkCategory category = getCategory(catId);
    category.setVisibility(!category.isVisible());
  }

  public Bookmark getBookmark(int catId, int bmkId)
  {
    return getCategory(catId).getBookmark(bmkId);
  }

  public Bookmark addNewBookmark(String name, double lat, double lon)
  {
    final Bookmark bookmark = nativeAddBookmarkToLastEditedCategory(name, lat, lon);
    Statistics.INSTANCE.trackBookmarkCreated();
    return bookmark;
  }

  public void addListener(BookmarksLoadingListener listener)
  {
    mListeners.add(listener);
  }

  public void removeListener(BookmarksLoadingListener listener)
  {
    mListeners.remove(listener);
  }

  // Called from JNI.
  @MainThread
  public void onBookmarksLoadingStarted()
  {
    for (BookmarksLoadingListener listener : mListeners)
      listener.onBookmarksLoadingStarted();
  }

  // Called from JNI.
  @MainThread
  public void onBookmarksLoadingFinished()
  {
    for (BookmarksLoadingListener listener : mListeners)
      listener.onBookmarksLoadingFinished();
  }

  // Called from JNI.
  @MainThread
  public void onBookmarksLoadingFile(boolean success, String fileName, boolean isTemporaryFile)
  {
    // Android could create temporary file with bookmarks in some cases. Here we can delete it.
    if (isTemporaryFile)
    {
      File tmpFile = new File(fileName);
      tmpFile.delete();
    }

    for (BookmarksLoadingListener listener : mListeners)
      listener.onBookmarksLoadingFile(success);
  }

  public static native void nativeLoadBookmarks();

  private native void nativeDeleteTrack(int catId, int trackId);

  private native void nativeDeleteBookmark(int cat, int bmkId);

  public native int nativeGetCategoriesCount();

  public native boolean nativeDeleteCategory(int catId);

  /**
   * @return category Id
   */
  public native int nativeCreateCategory(String name);

  public native void nativeShowBookmarkOnMap(int catId, int bmkId);

  /**
   * @return null, if wrong category is passed.
   */
  public native @Nullable String nativeSaveToKmzFile(int catId, String tmpPath);

  public native Bookmark nativeAddBookmarkToLastEditedCategory(String name, double lat, double lon);

  public native int nativeGetLastEditedCategory();

  public static native String nativeGenerateUniqueFileName(String baseName);

  public static native void nativeLoadKmzFile(String path, boolean isTemporaryFile);

  public static native String nativeFormatNewBookmarkName();

  public static native boolean nativeIsAsyncBookmarksLoadingInProgress();

  public interface BookmarksLoadingListener
  {
    void onBookmarksLoadingStarted();
    void onBookmarksLoadingFinished();

    void onBookmarksLoadingFile(boolean success);
  }
}
