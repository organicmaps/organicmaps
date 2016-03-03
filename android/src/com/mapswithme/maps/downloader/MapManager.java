package com.mapswithme.maps.downloader;

import android.support.annotation.Nullable;

import java.util.List;

import com.mapswithme.util.statistics.Statistics;

public final class MapManager
{
  @SuppressWarnings("unused")
  public static class StorageCallbackData
  {
    public final String countryId;
    public final int newStatus;
    public final boolean isLeafNode;

    public StorageCallbackData(String countryId, int newStatus, boolean isLeafNode)
    {
      this.countryId = countryId;
      this.newStatus = newStatus;
      this.isLeafNode = isLeafNode;
    }
  }

  @SuppressWarnings("unused")
  public interface StorageCallback
  {
    void onStatusChanged(List<StorageCallbackData> data);
    void onProgress(String countryId, long localSize, long remoteSize);
  }

  @SuppressWarnings("unused")
  interface CurrentCountryChangedListener
  {
    void onCurrentCountryChanged(String countryId);
  }

  @SuppressWarnings("unused")
  interface MigrationListener
  {
    void onComplete();
    void onProgress(int percent);
    void onError(int code);
  }

  private MapManager() {}

  public static void sendErrorStat(String event, int code)
  {
    String text;
    switch (code)
    {
    case CountryItem.ERROR_NO_INTERNET:
      text = "no_connection";
      break;

    case CountryItem.ERROR_OOM:
      text = "no_space";
      break;

    default:
      text = "unknown_error";
    }

    Statistics.INSTANCE.trackEvent(event, Statistics.params().add(Statistics.EventParam.TYPE, text));
  }

  /**
   * Moves a file from one place to another.
   */
  public static native boolean nativeMoveFile(String oldFile, String newFile);

  /**
   * Returns {@code true} if there is enough storage space to perform migration. Or {@code false} otherwise.
   */
  public static native boolean nativeHasSpaceForMigration();

  /**
   * Determines whether the legacy (large MWMs) mode is used.
   */
  public static native boolean nativeIsLegacyMode();

  /**
   * Performs migration from old (large MWMs) mode.
   * @return {@code true} if prefetch was started. {@code false} if maps were queued to downloader and migration process is complete.
   *         In this case {@link MigrationListener#onComplete()} will be called before return from {@code nativeMigrate()}.
   */
  public static native boolean nativeMigrate(MigrationListener listener, double lat, double lon, boolean hasLocation, boolean keepOldMaps);

  /**
   * Aborts migration. Affects only prefetch process.
   */
  public static native void nativeCancelMigration();

  /**
   * Returns country ID of the root node.
   * KILLME (trashkalmar): Unused?
   */
  public static native String nativeGetRootNode();

  /**
   * Return count of fully downloaded maps (excluding fake MWMs).
   */
  public static native int nativeGetDownloadedCount();

  /**
   * Returns info about updatable data or null on error.
   */
  public static native @Nullable UpdateInfo nativeGetUpdateInfo();

  /**
   * Retrieves list of country items with its status info. Uses root as parent if {@code root} is null.
   */
  public static native void nativeListItems(@Nullable String root, List<CountryItem> result);

  /**
   * Sets following attributes of the given {@code item}:
   * <pre>
   * <ul>
   *   <li>name;</li>
   *   <li>parentId;</li>
   *   <li>parentName;</li>
   *   <li>size;</li>
   *   <li>totalSize;</li>
   *   <li>childCount;</li>
   *   <li>totalChildCount;</li>
   *   <li>status;</li>
   *   <li>errorCode;</li>
   *   <li>present;</li>
   *   <li>progress</li>
   * </ul>
   * </pre>
   */
  public static native void nativeGetAttributes(CountryItem item);

  /**
   * Returns country ID corresponding to given coordinates or {@code null} on error.
   */
  public static native @Nullable String nativeFindCountry(double lat, double lon);

  /**
   * Determines whether something is downloading now.
   */
  public static native boolean nativeIsDownloading();

  /**
   * Enqueues given {@code root} node and its children in downloader.
   */
  public static native void nativeDownload(String root);

  /**
   * Enqueues failed items under given {@code root} node in downloader.
   */
  public static native void nativeRetry(String root);

  /**
   * Enqueues given {@code root} node with its children in downloader.
   */
  public static native void nativeUpdate(String root);

  /**
   * Removes given currently downloading {@code root} node and its children from downloader.
   */
  public static native void nativeCancel(String root);

  /**
   * Deletes given installed {@code root} node with its children.
   */
  public static native void nativeDelete(String root);

  /**
   * Registers {@code callback} of storage status changed. Returns slot ID which should be used to unsubscribe in {@link #nativeUnsubscribe(int)}.
   */
  public static native int nativeSubscribe(StorageCallback callback);

  /**
   * Unregisters storage status changed callback.
   * @param slot Slot ID returned from {@link #nativeSubscribe(StorageCallback)} while registering.
   */
  public static native void nativeUnsubscribe(int slot);

  /**
   * Sets callback about current country change. Single subscriber only.
   */
  public static native void nativeSubscribeOnCountryChanged(CurrentCountryChangedListener listener);

  /**
   * Removes callback about current country change.
   */
  public static native void nativeUnsubscribeOnCountryChanged();
}
