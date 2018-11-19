package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.IntDef;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.util.statistics.Statistics;

import java.io.File;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
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
            CLOUD_DISK_ERROR, CLOUD_USER_INTERRUPTED, CLOUD_INVALID_CALL })
  public @interface SynchronizationResult {}

  public static final int CLOUD_SUCCESS = 0;
  public static final int CLOUD_AUTH_ERROR = 1;
  public static final int CLOUD_NETWORK_ERROR = 2;
  public static final int CLOUD_DISK_ERROR = 3;
  public static final int CLOUD_USER_INTERRUPTED = 4;
  public static final int CLOUD_INVALID_CALL = 5;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ CLOUD_BACKUP_EXISTS, CLOUD_NO_BACKUP, CLOUD_NOT_ENOUGH_DISK_SPACE })
  public @interface RestoringRequestResult {}

  public static final int CLOUD_BACKUP_EXISTS = 0;
  public static final int CLOUD_NO_BACKUP = 1;
  public static final int CLOUD_NOT_ENOUGH_DISK_SPACE = 2;

  public static final List<Icon> ICONS = new ArrayList<>();

  @NonNull
  private final List<BookmarksLoadingListener> mListeners = new ArrayList<>();

  @NonNull
  private final List<KmlConversionListener> mConversionListeners = new ArrayList<>();

  @NonNull
  private final List<BookmarksSharingListener> mSharingListeners = new ArrayList<>();

  @NonNull
  private final List<BookmarksCloudListener> mCloudListeners = new ArrayList<>();

  @NonNull
  private final List<BookmarksCatalogListener> mCatalogListeners = new ArrayList<>();
  
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

  @NonNull
  Icon getIconByColor(@Icon.PredefinedColor int color)
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
    UserActionsLogger.logAddToBookmarkEvent();
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

  public void addCatalogListener(@NonNull BookmarksCatalogListener listener)
  {
    mCatalogListeners.add(listener);
  }

  public void removeCatalogListener(@NonNull BookmarksCatalogListener listener)
  {
    mCatalogListeners.remove(listener);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onBookmarksLoadingStarted()
  {
    for (BookmarksLoadingListener listener : mListeners)
      listener.onBookmarksLoadingStarted();
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onBookmarksLoadingFinished()
  {
    for (BookmarksLoadingListener listener : mListeners)
      listener.onBookmarksLoadingFinished();
  }

  // Called from JNI.
  @SuppressWarnings("unused")
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
  @SuppressWarnings("unused")
  @MainThread
  public void onFinishKmlConversion(boolean success)
  {
    for (KmlConversionListener listener : mConversionListeners)
      listener.onFinishKmlConversion(success);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onPreparedFileForSharing(BookmarkSharingResult result)
  {
    for (BookmarksSharingListener listener : mSharingListeners)
      listener.onPreparedFileForSharing(result);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onSynchronizationStarted(@SynchronizationType int type)
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onSynchronizationStarted(type);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onSynchronizationFinished(@SynchronizationType int type,
                                        @SynchronizationResult int result,
                                        @NonNull String errorString)
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onSynchronizationFinished(type, result, errorString);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onRestoreRequested(@RestoringRequestResult int result, @NonNull String deviceName,
                                 long backupTimestampInMs)
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onRestoreRequested(result, deviceName, backupTimestampInMs);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onRestoredFilesPrepared()
  {
    for (BookmarksCloudListener listener : mCloudListeners)
      listener.onRestoredFilesPrepared();
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onImportStarted(@NonNull String id)
  {
    for (BookmarksCatalogListener listener : mCatalogListeners)
      listener.onImportStarted(id);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onImportFinished(@NonNull String id, long catId, boolean successful)
  {
    if (successful)
      Statistics.INSTANCE.trackPurchaseProductDelivered(id, PrivateVariables.bookmarksVendor());
    for (BookmarksCatalogListener listener : mCatalogListeners)
      listener.onImportFinished(id, catId, successful);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onTagsReceived(boolean successful, @NonNull CatalogTagsGroup[] tagsGroups)
  {
    List<CatalogTagsGroup> unmodifiableData = Collections.unmodifiableList(Arrays.asList(tagsGroups));
    for (BookmarksCatalogListener listener : mCatalogListeners)
    {
      listener.onTagsReceived(successful, unmodifiableData);
    }
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onCustomPropertiesReceived(boolean successful,
                                         @NonNull CatalogCustomProperty[] properties)
  {
    List<CatalogCustomProperty> unmodifiableProperties = Collections.unmodifiableList(Arrays.asList(properties));
    for (BookmarksCatalogListener listener : mCatalogListeners)
      listener.onCustomPropertiesReceived(successful, unmodifiableProperties);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onUploadStarted(long originCategoryId)
  {
    for (BookmarksCatalogListener listener : mCatalogListeners)
      listener.onUploadStarted(originCategoryId);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  public void onUploadFinished(int index, @NonNull String description,
                               long originCategoryId, long resultCategoryId)
  {
    UploadResult result = UploadResult.values()[index];
    for (BookmarksCatalogListener listener : mCatalogListeners)
    {
      listener.onUploadFinished(result, description, originCategoryId, resultCategoryId);
    }
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

  public void setCategoryDescription(long id, @NonNull String categoryDesc)
  {
    nativeSetCategoryDescription(id, categoryDesc);
  }

  public void setCategoryTags(@NonNull BookmarkCategory category, @NonNull List<CatalogTag> tags)
  {
    String[] ids = new String[tags.size()];
    for (int i = 0; i < tags.size(); i++)
    {
      ids[i] = tags.get(i).getId();
    }
    nativeSetCategoryTags(category.getId(), ids);
  }

  public void setCategoryProperties(@NonNull BookmarkCategory category,
                                    @NonNull List<CatalogPropertyOptionAndKey> properties)
  {
    for (CatalogPropertyOptionAndKey each : properties)
    {
      nativeSetCategoryCustomProperty(category.getId(), each.getKey(), each.getOption().getValue());
    }
  }

  public void setAccessRules(long id, @NonNull BookmarkCategory.AccessRules rules)
  {
    nativeSetCategoryAccessRules(id, rules.ordinal());
  }

  public void uploadToCatalog(@NonNull BookmarkCategory.AccessRules rules, @NonNull BookmarkCategory category)
  {
    nativeUploadToCatalog(rules.ordinal(), category.getId());
  }

  /**
   * @return total count - tracks + bookmarks
   * @param category
   */
  @Deprecated
  public int getCategorySize(@NonNull BookmarkCategory category)
  {
    return nativeGetBookmarksCount(category.getId()) + nativeGetTracksCount(category.getId());
  }

  public int getCategoriesCount() { return nativeGetCategoriesCount(); }

  @NonNull
  public BookmarkCategory getCategoryById(long catId)
  {
    List<BookmarkCategory> items = getAllCategoriesSnapshot().getItems();
    for (BookmarkCategory each : items)
    {
      if (catId == each.getId())
        return each;
    }
    throw new IllegalArgumentException(new StringBuilder().append("Category with id = ")
                                                          .append(catId)
                                                          .append(" missed")
                                                          .toString());
  }

  @Deprecated
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

  @NonNull
  public AbstractCategoriesSnapshot.Default getDownloadedCategoriesSnapshot()
  {
    BookmarkCategory[] items = nativeGetBookmarkCategories();
    return new AbstractCategoriesSnapshot.Default(items, new FilterStrategy.Downloaded());
  }

  @NonNull
  public AbstractCategoriesSnapshot.Default getOwnedCategoriesSnapshot()
  {
    BookmarkCategory[] items = nativeGetBookmarkCategories();
    return new AbstractCategoriesSnapshot.Default(items, new FilterStrategy.Private());
  }

  @NonNull
  public AbstractCategoriesSnapshot.Default getAllCategoriesSnapshot()
  {
    BookmarkCategory[] items = nativeGetBookmarkCategories();
    return new AbstractCategoriesSnapshot.Default(items, new FilterStrategy.All());
  }

  @NonNull
  public AbstractCategoriesSnapshot.Default getCategoriesSnapshot(FilterStrategy strategy)
  {
    return new AbstractCategoriesSnapshot.Default(nativeGetBookmarkCategories(), strategy);
  }

  public boolean isUsedCategoryName(@NonNull String name)
  {
    return nativeIsUsedCategoryName(name);
  }

  public boolean isEditableBookmark(long bmkId) { return nativeIsEditableBookmark(bmkId); }

  public boolean isEditableTrack(long trackId) { return nativeIsEditableTrack(trackId); }

  public boolean isEditableCategory(long catId) { return nativeIsEditableCategory(catId); }

  public boolean areAllCatalogCategoriesVisible()
  {
    return areAllCategoriesVisible(BookmarkCategory.Type.DOWNLOADED);
  }

  public boolean areAllOwnedCategoriesVisible()
  {
    return areAllCategoriesVisible(BookmarkCategory.Type.PRIVATE);
  }

  public boolean areAllCategoriesVisible(BookmarkCategory.Type type)
  {
    return nativeAreAllCategoriesVisible(type.ordinal());
  }

  public boolean areAllCategoriesInvisible(BookmarkCategory.Type type)
  {
    return nativeAreAllCategoriesInvisible(type.ordinal());
  }

  public boolean areAllCatalogCategoriesInvisible()
  {
    return areAllCategoriesInvisible(BookmarkCategory.Type.DOWNLOADED);
  }

  public boolean areAllOwnedCategoriesInvisible()
  {
    return areAllCategoriesInvisible(BookmarkCategory.Type.PRIVATE);
  }

  public void setAllCategoriesVisibility(boolean visible, @NonNull BookmarkCategory.Type type)
  {
    nativeSetAllCategoriesVisibility(visible, type.ordinal());
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

  public void setNotificationsEnabled(boolean enabled)
  {
    nativeSetNotificationsEnabled(enabled);
  }

  public boolean areNotificationsEnabled()
  {
    return nativeAreNotificationsEnabled();
  }

  public void importFromCatalog(@NonNull String serverId, @NonNull String filePath)
  {
    nativeImportFromCatalog(serverId, filePath);
  }

  public void uploadRoutes(int accessRules, @NonNull BookmarkCategory bookmarkCategory)
  {
    nativeUploadToCatalog(accessRules, bookmarkCategory.getId());
  }

  @NonNull
  public String getCatalogDeeplink(long catId)
  {
    return nativeGetCatalogDeeplink(catId);
  }

  @NonNull
  public String getCatalogDownloadUrl(@NonNull String serverId)
  {
    return nativeGetCatalogDownloadUrl(serverId);
  }

  @NonNull
  public String getCatalogFrontendUrl()
  {
    return nativeGetCatalogFrontendUrl();
  }

  public void requestRouteTags()
  {
    nativeRequestCatalogTags();
  }

  public void requestCustomProperties()
  {
    nativeRequestCatalogCustomProperties();
  }

  public boolean isCategoryFromCatalog(long catId)
  {
    return nativeIsCategoryFromCatalog(catId);
  }

  @NonNull
  private String getCategoryAuthor(long catId)
  {
    return nativeGetCategoryAuthor(catId);
  }

  private native int nativeGetCategoriesCount();

  private native int nativeGetCategoryPositionById(long catId);

  private native long nativeGetCategoryIdByPosition(int position);

  private native int nativeGetBookmarksCount(long catId);

  private native int nativeGetTracksCount(long catId);

  private native BookmarkCategory[] nativeGetBookmarkCategories();

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

  private native void nativeSetCategoryDescription(long catId, @NonNull String desc);

  private native void nativeSetCategoryTags(long catId, @NonNull String[] tagsIds);

  private native void nativeSetCategoryAccessRules(long catId, int accessRules);

  private native void nativeSetCategoryCustomProperty(long catId, String key, String value);

  @NonNull
  private native String nativeGetCategoryAuthor(long catId);

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

  private static native boolean nativeIsEditableBookmark(long bmkId);

  private static native boolean nativeIsEditableTrack(long trackId);

  private static native boolean nativeIsEditableCategory(long catId);

  private static native boolean nativeAreAllCategoriesVisible(int type);

  private static native boolean nativeAreAllCategoriesInvisible(int type);

  private static native void nativeSetAllCategoriesVisibility(boolean visible, int type);

  private static native int nativeGetKmlFilesCountForConversion();

  private static native void nativeConvertAllKmlFiles();

  private static native void nativePrepareFileForSharing(long catId);

  private static native boolean nativeIsCategoryEmpty(long catId);

  private static native void nativeRequestRestoring();

  private static native void nativeApplyRestoring();

  private static native void nativeCancelRestoring();

  private static native void nativeSetNotificationsEnabled(boolean enabled);

  private static native boolean nativeAreNotificationsEnabled();

  private static native void nativeImportFromCatalog(@NonNull String serverId,
                                                     @NonNull String filePath);

  private static native void nativeUploadToCatalog(int accessRules,
                                                   long catId);

  @NonNull
  private static native String nativeGetCatalogDeeplink(long catId);

  @NonNull
  private static native String nativeGetCatalogDownloadUrl(@NonNull String serverId);

  @NonNull
  private static native String nativeGetCatalogFrontendUrl();

  private static native boolean nativeIsCategoryFromCatalog(long catId);

  private static native void nativeRequestCatalogTags();

  private static native void nativeRequestCatalogCustomProperties();

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
     * @param deviceName The name of device which was the source of the backup.
     * @param backupTimestampInMs contains timestamp of the backup on the server (in milliseconds).
     */
    void onRestoreRequested(@RestoringRequestResult int result, @NonNull String deviceName,
                            long backupTimestampInMs);

    /**
     * Restored bookmark files are prepared to substitute for the current ones.
     * After this callback any cached bookmarks data become invalid. Also after this
     * callback the restoring process can not be cancelled.
     */
    void onRestoredFilesPrepared();
  }

  public interface BookmarksCatalogListener
  {
    /**
     * The method is called when the importing of a file from the catalog is started.
     *
     * @param serverId is server identifier of the file.
     */
    void onImportStarted(@NonNull String serverId);

    /**
     * The method is called when the importing of a file from the catalog is finished.
     *
     * @param serverId is server identifier of the file.
     * @param catId is client identifier of the created bookmarks category.
     * @param successful is result of the importing.
     */
    void onImportFinished(@NonNull String serverId, long catId, boolean successful);

    /**
     * The method is called when the tags were received from the server.
     *  @param successful is the result of the receiving.
     * @param tagsGroups is the tags collection.
     */
    void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups);

    /**
     * The method is called when the custom properties were received from the server.
     *  @param successful is the result of the receiving.
     * @param properties is the properties collection.
     */
    void onCustomPropertiesReceived(boolean successful,
                                    @NonNull List<CatalogCustomProperty> properties);

    /**
     * The method is called when the uploading to the catalog is started.
     *
     * @param originCategoryId is identifier of the uploading bookmarks category.
     */
    void onUploadStarted(long originCategoryId);

    /**
     * The method is called when the uploading to the catalog is finished.
     *  @param uploadResult is result of the uploading.
     * @param description is detailed description of the uploading result.
     * @param originCategoryId is original identifier of the uploaded bookmarks category.
     * @param resultCategoryId is identifier of the uploaded category after finishing.
*                         In the case of bookmarks modification during uploading
     */
    void onUploadFinished(@NonNull UploadResult uploadResult, @NonNull String description,
                          long originCategoryId, long resultCategoryId);
  }

  public static class DefaultBookmarksCatalogListener implements BookmarksCatalogListener
  {
    @Override
    public void onImportStarted(@NonNull String serverId)
    {
      /* do noting by default */
    }

    @Override
    public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
    {
      /* do noting by default */
    }

    @Override
    public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups)
    {
      /* do noting by default */
    }

    @Override
    public void onCustomPropertiesReceived(boolean successful,
                                           @NonNull List<CatalogCustomProperty> properties)
    {
      /* do noting by default */
    }

    @Override
    public void onUploadStarted(long originCategoryId)
    {
      /* do noting by default */
    }

    @Override
    public void onUploadFinished(@NonNull UploadResult uploadResult, @NonNull String description,
                                 long originCategoryId, long resultCategoryId)
    {
      /* do noting by default */
    }
  }

  public enum UploadResult
  {
    UPLOAD_RESULT_SUCCESS,
    UPLOAD_RESULT_NETWORK_ERROR,
    UPLOAD_RESULT_SERVER_ERROR,
    UPLOAD_RESULT_AUTH_ERROR,
    /* Broken file */
    UPLOAD_RESULT_MALFORMED_DATA_ERROR,
    /* Edit on web */
    UPLOAD_RESULT_ACCESS_ERROR,
    UPLOAD_RESULT_INVALID_CALL;
  }
}
