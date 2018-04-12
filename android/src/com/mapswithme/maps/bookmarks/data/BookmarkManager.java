package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.IntDef;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

import java.io.File;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

@MainThread
public enum BookmarkManager
{
  INSTANCE;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ CLOUD_BACKUP, CLOUD_RESTORE })
  public @interface SynchronizationType {}

  public static final int CLOUD_BACKUP = 0;
  public static final int CLOUD_RESTORE = 1;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ CLOUD_SUCCESS, CLOUD_AUTH_ERROR, CLOUD_NETWORK_ERROR,
            CLOUD_DISK_ERROR, CLOUD_USER_INTERRUPTED })
  public @interface SynchronizationResult {}

  public static final int CLOUD_SUCCESS = 0;
  public static final int CLOUD_AUTH_ERROR = 1;
  public static final int CLOUD_NETWORK_ERROR = 2;
  public static final int CLOUD_DISK_ERROR = 3;
  public static final int CLOUD_USER_INTERRUPTED = 4;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ CLOUD_BACKUP_EXISTS, CLOUD_NO_BACKUP, CLOUD_NOT_ENOUGH_DISK_SPACE })
  public @interface RestoringRequestResult {}

  public static final int CLOUD_BACKUP_EXISTS = 0;
  public static final int CLOUD_NO_BACKUP = 1;
  public static final int CLOUD_NOT_ENOUGH_DISK_SPACE = 2;

  public static final List<Icon> ICONS = new ArrayList<>();

  @NonNull
  private List<BookmarksLoadingListener> mListeners = new ArrayList<>();

  @NonNull
  private List<KmlConversionListener> mConversionListeners = new ArrayList<>();

  @NonNull
  private List<BookmarksSharingListener> mSharingListeners = new ArrayList<>();

  @NonNull
  private List<BookmarksCloudListener> mCloudListeners = new ArrayList<>();
  
  static
  {
    ICONS.add(new Icon("placemark-red", Icon.PREDEFINED_COLOR_RED, R.drawable.ic_bookmark_marker_red_off, R.drawable.ic_bookmark_marker_red_on));
    ICONS.add(new Icon("placemark-blue", Icon.PREDEFINED_COLOR_BLUE, R.drawable.ic_bookmark_marker_blue_off, R.drawable.ic_bookmark_marker_blue_on));
    ICONS.add(new Icon("placemark-purple", Icon.PREDEFINED_COLOR_PURPLE, R.drawable.ic_bookmark_marker_purple_off, R.drawable.ic_bookmark_marker_purple_on));
    ICONS.add(new Icon("placemark-yellow", Icon.PREDEFINED_COLOR_YELLOW, R.drawable.ic_bookmark_marker_yellow_off, R.drawable.ic_bookmark_marker_yellow_on));
    ICONS.add(new Icon("placemark-pink", Icon.PREDEFINED_COLOR_PINK, R.drawable.ic_bookmark_marker_pink_off, R.drawable.ic_bookmark_marker_pink_on));
    ICONS.add(new Icon("placemark-brown", Icon.PREDEFINED_COLOR_BROWN, R.drawable.ic_bookmark_marker_brown_off, R.drawable.ic_bookmark_marker_brown_on));
    ICONS.add(new Icon("placemark-green", Icon.PREDEFINED_COLOR_GREEN, R.drawable.ic_bookmark_marker_green_off, R.drawable.ic_bookmark_marker_green_on));
    ICONS.add(new Icon("placemark-orange", Icon.PREDEFINED_COLOR_ORANGE, R.drawable.ic_bookmark_marker_orange_off, R.drawable.ic_bookmark_marker_orange_on));
  }

  static Icon getIconByColor(@Icon.PredefinedColor int color)
  {
    for (Icon icon : ICONS)
    {
      if (icon.getColor() == color)
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

  public Bookmark addNewBookmark(double lat, double lon)
  {
    final Bookmark bookmark = nativeAddBookmarkToLastEditedCategory(lat, lon);
    Statistics.INSTANCE.trackBookmarkCreated();
    return bookmark;
  }

  public void addLoadingListener(@NonNull BookmarksLoadingListener listener)
  {
    mListeners.add(listener);
  }

  public void removeLoadingListener(@NonNull BookmarksLoadingListener listener)
  {
    mListeners.remove(listener);
  }

  public void addKmlConversionListener(@NonNull KmlConversionListener listener)
  {
    mConversionListeners.add(listener);
  }

  public void removeKmlConversionListener(@NonNull KmlConversionListener listener)
  {
    mConversionListeners.remove(listener);
  }

  public void addSharingListener(@NonNull BookmarksSharingListener listener)
  {
    mSharingListeners.add(listener);
  }

  public void removeSharingListener(@NonNull BookmarksSharingListener listener)
  {
    mSharingListeners.remove(listener);
  }

  public void addCloudListener(@NonNull BookmarksCloudListener listener)
  {
    mCloudListeners.add(listener);
  }

  public void removeCloudListener(@NonNull BookmarksCloudListener listener)
  {
    mCloudListeners.remove(listener);
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

  // Called from JNI.
  @MainThread
  public void onFinishKmlConversion(boolean success)
  {
    for (KmlConversionListener listener : mConversionListeners)
      listener.onFinishKmlConversion(success);
  }

  // Called from JNI.
  @MainThread
  public void onPreparedFileForSharing(BookmarkSharingResult result)
  {
    for (BookmarksSharingListener listener : mSharingListeners)
      listener.onPreparedFileForSharing(result);
  }

  // Called from JNI.
  @MainThread
  public void onSynchronizationStarted(@SynchronizationType int type)
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onSynchronizationStarted(type);
  }

  // Called from JNI.
  @MainThread
  public void onSynchronizationFinished(@SynchronizationType int type,
                                        @SynchronizationResult int result,
                                        @NonNull String errorString)
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onSynchronizationFinished(type, result, errorString);
  }

  // Called from JNI.
  @MainThread
  public void onRestoreRequested(@RestoringRequestResult int result,
                                 long backupTimestampInMs)
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onRestoreRequested(result, backupTimestampInMs);
  }

  // Called from JNI.
  @MainThread
  public void onRestoredFilesPrepared()
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onRestoredFilesPrepared();
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

  public long getLastEditedCategory() { return nativeGetLastEditedCategory(); }

  @Icon.PredefinedColor
  public int getLastEditedColor() { return nativeGetLastEditedColor(); }

  public void setCloudEnabled(boolean enabled) { nativeSetCloudEnabled(enabled); }

  public boolean isCloudEnabled() { return nativeIsCloudEnabled(); }

  public long getLastSynchronizationTimestampInMs()
  {
    return nativeGetLastSynchronizationTimestampInMs();
  }

  public void loadKmzFile(@NonNull String path, boolean isTemporaryFile)
  {
    nativeLoadKmzFile(path, isTemporaryFile);
  }

  public boolean isAsyncBookmarksLoadingInProgress()
  {
    return nativeIsAsyncBookmarksLoadingInProgress();
  }

  public boolean isUsedCategoryName(@NonNull String name)
  {
    return nativeIsUsedCategoryName(name);
  }

  public boolean areAllCategoriesVisible()
  {
    return nativeAreAllCategoriesVisible();
  }

  public boolean areAllCategoriesInvisible()
  {
    return nativeAreAllCategoriesInvisible();
  }

  public void setAllCategoriesVisibility(boolean visible)
  {
    nativeSetAllCategoriesVisibility(visible);
  }

  public int getKmlFilesCountForConversion()
  {
    return nativeGetKmlFilesCountForConversion();
  }

  public void convertAllKmlFiles()
  {
    nativeConvertAllKmlFiles();
  }

  public void prepareFileForSharing(long catId)
  {
    nativePrepareFileForSharing(catId);
  }

  public boolean isCategoryEmpty(long catId)
  {
    return nativeIsCategoryEmpty(catId);
  }

  public void prepareCategoryForSharing(long catId)
  {
    nativePrepareFileForSharing(catId);
  }

  public void requestRestoring()
  {
    nativeRequestRestoring();
  }

  public void applyRestoring()
  {
    nativeApplyRestoring();
  }

  public void cancelRestoring()
  {
    nativeCancelRestoring();
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

  @NonNull
  private native Bookmark nativeAddBookmarkToLastEditedCategory(double lat, double lon);

  private native long nativeGetLastEditedCategory();

  @Icon.PredefinedColor
  private native int nativeGetLastEditedColor();

  private native void nativeSetCloudEnabled(boolean enabled);

  private native boolean nativeIsCloudEnabled();

  private native long nativeGetLastSynchronizationTimestampInMs();

  private static native void nativeLoadKmzFile(@NonNull String path, boolean isTemporaryFile);

  private static native boolean nativeIsAsyncBookmarksLoadingInProgress();

  private static native boolean nativeIsUsedCategoryName(@NonNull String name);

  private static native boolean nativeAreAllCategoriesVisible();

  private static native boolean nativeAreAllCategoriesInvisible();

  private static native void nativeSetAllCategoriesVisibility(boolean visible);

  private static native int nativeGetKmlFilesCountForConversion();

  private static native void nativeConvertAllKmlFiles();

  private static native void nativePrepareFileForSharing(long catId);

  private static native boolean nativeIsCategoryEmpty(long catId);

  private static native void nativeRequestRestoring();

  private static native void nativeApplyRestoring();

  private static native void nativeCancelRestoring();

  public interface BookmarksLoadingListener
  {
    void onBookmarksLoadingStarted();
    void onBookmarksLoadingFinished();
    void onBookmarksFileLoaded(boolean success);
  }

  public interface KmlConversionListener
  {
    void onFinishKmlConversion(boolean success);
  }

  public interface BookmarksSharingListener
  {
    void onPreparedFileForSharing(@NonNull BookmarkSharingResult result);
  }

  public interface BookmarksCloudListener
  {
    /**
     * The method is called when the synchronization started.
     *
     * @param type determines type of synchronization (backup or restoring).
     */
    void onSynchronizationStarted(@SynchronizationType int type);

    /**
     * The method is called when the synchronization finished.
     *
     * @param type determines type of synchronization (backup or restoring).
     * @param result is one of possible results of the synchronization.
     * @param errorString contains detailed description in case of unsuccessful completion.
     */
    void onSynchronizationFinished(@SynchronizationType int type,
                                   @SynchronizationResult int result,
                                   @NonNull String errorString);

    /**
     * The method is called after restoring request.
     *
     * @param result By result you can determine if the restoring is possible.
     * @param backupTimestampInMs contains timestamp of the backup on the server (in milliseconds).
     */
    void onRestoreRequested(@RestoringRequestResult int result, long backupTimestampInMs);

    /**
     * Restored bookmark files are prepared to substitute for the current ones.
     * After this callback any cached bookmarks data become invalid. Also after this
     * callback the restoring process can not be cancelled.
     */
    void onRestoredFilesPrepared();
  }
}
