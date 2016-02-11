package com.mapswithme.maps.downloader;

import android.support.annotation.Nullable;

import java.util.List;

public final class MapManager
{
  @SuppressWarnings("unused")
  public interface StorageCallback
  {
    void onStatusChanged(String countryId, int newStatus);
    void onProgress(String countryId, long localSize, long remoteSize);
  }

  private MapManager() {}

  /**
   * Moves a file from one place to another.
   */
  public static native boolean nativeMoveFile(String oldFile, String newFile);

  /**
   * Determines whether the legacy (large MWMs) mode is used.
   */
  public static native boolean nativeIsLegacyMode();

  /**
   * Performs migration from old (large MWMs) mode.
   */
  public static native void nativeMigrate();

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
   *   <li>size;</li>
   *   <li>totalSize;</li>
   *   <li>childCount;</li>
   *   <li>totalChildCount;</li>
   *   <li>status;</li>
   *   <li>errorCode;</li>
   *   <li>present</li>
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
  public static native boolean nativeDownload(String root);

  /**
   * Enqueues failed items under given {@code root} node in downloader.
   */
  public static native boolean nativeRetry(String root);

  /**
   * Enqueues given {@code root} node with its children in downloader.
   */
  public static native boolean nativeUpdate(String root);

  /**
   * Removes given currently downloading {@code root} node and its children from downloader.
   */
  public static native boolean nativeCancel(String root);

  /**
   * Deletes given installed {@code root} node with its children.
   */
  public static native boolean nativeDelete(String root);

  /**
   * Registers {@code callback} of storage status changed. Returns slot ID which should be used to unsubscribe in {@link #nativeUnsubscribe(int)}.
   */
  public static native int nativeSubscribe(StorageCallback callback);

  /**
   * Unregisters storage status changed callback.
   * @param slot Slot ID returned from {@link #nativeSubscribe(StorageCallback)} while registering.
   */
  public static native void nativeUnsubscribe(int slot);
}
