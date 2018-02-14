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
public enum BookmarkManager
{
  INSTANCE;

  public static final List<Icon> ICONS = new ArrayList<>();

  @NonNull
  private List<BookmarksLoadingListener> mListeners = new ArrayList<>();

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

  public void toggleCategoryVisibility(long catId)
  {
    boolean isVisible = isVisible(catId);
    setVisibility(catId, !isVisible);
  }

  public Bookmark addNewBookmark(String name, double lat, double lon)
  {
    final Bookmark bookmark = nativeAddBookmarkToLastEditedCategory(name, lat, lon);
    Statistics.INSTANCE.trackBookmarkCreated();
    return bookmark;
  }

  public void addListener(@NonNull BookmarksLoadingListener listener)
  {
    mListeners.add(listener);
  }

  public void removeListener(@NonNull BookmarksLoadingListener listener)
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
  public void onBookmarksFileLoaded(boolean success, @NonNull String fileName,
                                    boolean isTemporaryFile)
  {
    // Android could create temporary file with bookmarks in some cases (KML/KMZ file is a blob
    // in the intent, so we have to create a temporary file on the disk). Here we can delete it.
    if (isTemporaryFile)
    {
      File tmpFile = new File(fileName);
      tmpFile.delete();
    }

    for (BookmarksLoadingListener listener : mListeners)
      listener.onBookmarksFileLoaded(success);
  }

  public boolean isVisible(long catId)
  {
    return nativeIsVisible(catId);
  }

  public void setVisibility(long catId, boolean visible)
  {
    nativeSetVisibility(catId, visible);
  }

  @NonNull
  public String getCategoryName(long catId)
  {
    return nativeGetCategoryName(catId);
  }

  public void setCategoryName(long catId, @NonNull String name)
  {
    nativeSetCategoryName(catId, name);
  }

  /**
   * @return total count - tracks + bookmarks
   */
  public int getCategorySize(long catId)
  {
    return nativeGetBookmarksCount(catId) + nativeGetTracksCount(catId);
  }

  public int getCategoriesCount() { return nativeGetCategoriesCount(); }

  public int getCategoryPositionById(long catId)
  {
    return nativeGetCategoryPositionById(catId);
  }

  public long getCategoryIdByPosition(int position)
  {
    return nativeGetCategoryIdByPosition(position);
  }

  public int getBookmarksCount(long catId)
  {
    return nativeGetBookmarksCount(catId);
  }

  public int getTracksCount(long catId)
  {
    return nativeGetTracksCount(catId);
  }

  @NonNull
  public Bookmark getBookmark(long bmkId)
  {
    return nativeGetBookmark(bmkId);
  }

  public long getBookmarkIdByPosition(long catId, int positionInCategory)
  {
    return nativeGetBookmarkIdByPosition(catId, positionInCategory);
  }

  @NonNull
  public Track getTrack(long trackId)
  {
    return nativeGetTrack(trackId, Track.class);
  }

  public long getTrackIdByPosition(long catId, int positionInCategory)
  {
    return nativeGetTrackIdByPosition(catId, positionInCategory);
  }

  public static void loadBookmarks() { nativeLoadBookmarks(); }

  public void deleteCategory(long catId) { nativeDeleteCategory(catId); }

  public void deleteTrack(long trackId)
  {
    nativeDeleteTrack(trackId);
  }

  public void deleteBookmark(long bmkId)
  {
    nativeDeleteBookmark(bmkId);
  }

  public long createCategory(@NonNull String name) { return nativeCreateCategory(name); }

  public void showBookmarkOnMap(long bmkId) { nativeShowBookmarkOnMap(bmkId); }

  /**
   * @return null, if wrong category is passed.
   */
  @Nullable
  public String saveToKmzFile(long catId, @NonNull String tmpPath)
  {
    return nativeSaveToKmzFile(catId, tmpPath);
  }

  @NonNull
  public Bookmark addBookmarkToLastEditedCategory(@NonNull String name, double lat, double lon)
  {
    return nativeAddBookmarkToLastEditedCategory(name, lat, lon);
  }

  public long getLastEditedCategory() { return nativeGetLastEditedCategory(); }

  @NonNull
  public static String generateUniqueFileName(@NonNull String baseName)
  {
    return nativeGenerateUniqueFileName(baseName);
  }

  public void setCloudEnabled(boolean enabled) { nativeSetCloudEnabled(enabled); }

  public boolean isCloudEnabled() { return nativeIsCloudEnabled(); }

  public long getLastSynchronizationTimestampInMs()
  {
    return nativeGetLastSynchronizationTimestampInMs();
  }

  public static void loadKmzFile(@NonNull String path, boolean isTemporaryFile)
  {
    nativeLoadKmzFile(path, isTemporaryFile);
  }

  @NonNull
  public static String formatNewBookmarkName()
  {
    return nativeFormatNewBookmarkName();
  }

  public static boolean isAsyncBookmarksLoadingInProgress()
  {
    return nativeIsAsyncBookmarksLoadingInProgress();
  }

  private native int nativeGetCategoriesCount();

  private native int nativeGetCategoryPositionById(long catId);

  private native long nativeGetCategoryIdByPosition(int position);

  private native int nativeGetBookmarksCount(long catId);

  private native int nativeGetTracksCount(long catId);

  @NonNull
  private native Bookmark nativeGetBookmark(long bmkId);

  private native long nativeGetBookmarkIdByPosition(long catId, int position);

  @NonNull
  private native Track nativeGetTrack(long trackId, Class<Track> trackClazz);

  private native long nativeGetTrackIdByPosition(long catId, int position);

  private native boolean nativeIsVisible(long catId);

  private native void nativeSetVisibility(long catId, boolean visible);

  @NonNull
  private native String nativeGetCategoryName(long catId);

  private native void nativeSetCategoryName(long catId, @NonNull String n);

  private static native void nativeLoadBookmarks();

  private native boolean nativeDeleteCategory(long catId);

  private native void nativeDeleteTrack(long trackId);

  private native void nativeDeleteBookmark(long bmkId);

  /**
   * @return category Id
   */
  private native long nativeCreateCategory(@NonNull String name);

  private native void nativeShowBookmarkOnMap(long bmkId);

  /**
   * @return null, if wrong category is passed.
   */
  @Nullable
  private native String nativeSaveToKmzFile(long catId, @NonNull String tmpPath);

  @NonNull
  private native Bookmark nativeAddBookmarkToLastEditedCategory(String name, double lat, double lon);

  private native long nativeGetLastEditedCategory();

  private native void nativeSetCloudEnabled(boolean enabled);

  private native boolean nativeIsCloudEnabled();

  private native long nativeGetLastSynchronizationTimestampInMs();

  @NonNull
  private static native String nativeGenerateUniqueFileName(@NonNull String baseName);

  private static native void nativeLoadKmzFile(@NonNull String path, boolean isTemporaryFile);

  @NonNull
  private static native String nativeFormatNewBookmarkName();

  private static native boolean nativeIsAsyncBookmarksLoadingInProgress();

  public interface BookmarksLoadingListener
  {
    void onBookmarksLoadingStarted();
    void onBookmarksLoadingFinished();
    void onBookmarksFileLoaded(boolean success);
  }
}
