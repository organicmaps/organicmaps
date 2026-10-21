package app.organicmaps.sdk.downloader;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import java.util.List;

/**
 * Main-thread access to the native offline-map catalogue and download queue.
 * Wait for map-loading completion before querying it. Country IDs are catalogue identifiers, not localized names.
 * Downloads may outlive an activity; remove its status subscription when it stops and read current state on return.
 * Bundled World maps provide an overview; detailed search and routing require downloaded region maps.
 */
@UiThread
public final class MapManager
{
  /**
   * Snapshot of a catalogue node status change. Sizes and current attributes can be queried separately.
   */
  @Keep
  public static class StorageCallbackData
  {
    public final String countryId;
    public final int newStatus;
    public final int errorCode;
    public final boolean isLeafNode;

    /**
     * Creates a status snapshot.
     * @param countryId catalogue node identifier
     * @param newStatus a {@link CountryItem} {@code STATUS_*} value
     * @param errorCode a {@link CountryItem} {@code ERROR_*} value
     * @param isLeafNode whether this is one downloadable map rather than a group
     */
    public StorageCallbackData(String countryId, int newStatus, int errorCode, boolean isLeafNode)
    {
      this.countryId = countryId;
      this.newStatus = newStatus;
      this.errorCode = errorCode;
      this.isLeafNode = isLeafNode;
    }
  }

  // Used by JNI. Completed parts remain in the snapshot until the country/group batch finishes.
  @Keep
  public static final class Progress
  {
    public final long mwmDownloaded;
    public final long mwmTotal;
    public final long twmDownloaded;
    public final long twmTotal;

    public Progress(long mwmDownloaded, long mwmTotal, long twmDownloaded, long twmTotal)
    {
      this.mwmDownloaded = mwmDownloaded;
      this.mwmTotal = mwmTotal;
      this.twmDownloaded = twmDownloaded;
      this.twmTotal = twmTotal;
    }

    public long getDownloadedBytes()
    {
      return mwmDownloaded + twmDownloaded;
    }

    public long getTotalBytes()
    {
      return mwmTotal + twmTotal;
    }
  }

  /**
   * Main-thread download callbacks registered with {@link #nativeSubscribe}.
   */
  public interface StorageCallback
  {
    /**
     * Reports changed node statuses. Refresh affected nodes rather than assuming a single leaf changed.
     */
    @Keep
    void onStatusChanged(List<StorageCallbackData> data);

    /**
     * Reports independent map and terrain byte counts for a catalogue node; guard a zero total in progress UI.
     */
    @Keep
    void onProgress(String countryId, Progress progress);
  }

  /**
   * Listener for the region selected by the native map position; only one is installed at a time.
   */
  public interface CurrentCountryChangedListener
  {
    /**
     * Reports the current catalogue region identifier.
     */
    @Keep
    @SuppressWarnings("unused")
    void onCurrentCountryChanged(String countryId);
  }

  private MapManager() {}

  /**
   * Requeues failed downloads under a catalogue node.
   * @param countryId node or group identifier
   */
  public static void retryDownload(@NonNull String countryId)
  {
    nativeRetry(countryId);
  }

  /**
   * Queues available updates for a catalogue node and its children.
   * @param root node or group identifier
   */
  public static void startUpdate(@NonNull String root)
  {
    nativeUpdate(root);
  }

  /**
   * Queues the supplied catalogue nodes and their children.
   * @param countries node identifiers obtained from the catalogue or region-name search
   */
  public static void startDownload(@NonNull String[] countries, boolean withTerrain)
  {
    for (var countryId : countries)
    {
      nativeDownload(countryId, withTerrain);
    }
  }

  /**
   * Queues a catalogue node and its children. Observe progress and completion through {@link StorageCallback}.
   * @param countryId node or group identifier
   */
  public static void startDownload(@NonNull String countryId)
  {
    nativeDownload(countryId, true);
  }

  /**
   * Retrieves ID of root node.
   */
  public static native String nativeGetRoot();

  /**
   * Moves a file from one place to another.
   */
  public static native boolean nativeMoveFile(String oldFile, String newFile);

  /**
   * Returns {@code true} if there is enough storage space to download specified amount of data. Or {@code false}
   * otherwise.
   */
  public static native boolean nativeHasSpaceToDownloadAmount(long bytes);

  /**
   * Returns {@code true} if there is enough storage space to download maps with specified {@code root}. Or {@code
   * false} otherwise.
   */
  public static native boolean nativeHasSpaceToDownloadCountry(String root);

  /** Bytes of the maps not queued or downloading yet, plus their unqueued missing terrain unless disabled. */
  public static native long nativeGetDownloadSize(@NonNull String[] countries, boolean withTerrain);

  /**
   * Returns {@code true} if there is enough storage space to update maps with specified {@code root}. Or {@code false}
   * otherwise.
   */
  public static native boolean nativeHasSpaceToUpdate(String root);

  /**
   * Return count of fully downloaded maps (excluding fake MWMs).
   */
  public static native int nativeGetDownloadedCount();

  /**
   * Returns info about updatable data under given {@code root} or null on error.
   */
  public static native @Nullable UpdateInfo nativeGetUpdateInfo(@Nullable String root);

  /**
   * Retrieves list of country items with its status info.
   * if {@code root} is {@code null}, list of downloaded countries is returned.
   */
  public static native void nativeListItems(@Nullable String root, double lat, double lon, boolean hasLocation,
                                            boolean myMapsMode, List<CountryItem> result);

  /**
   * Sets following attributes of the given {@code item}:
   * <pre>
   * <ul>
   *   <li>name;</li>
   *   <li>directParentId;</li>
   *   <li>topmostParentId;</li>
   *   <li>directParentName;</li>
   *   <li>topmostParentName;</li>
   *   <li>description;</li>
   *   <li>size;</li>
   *   <li>downloadSize;</li>
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
   * Returns status for given {@code root} node.
   */
  public static native int nativeGetStatus(String root);

  /** Whether the MWM is current, regardless of terrain availability. */
  public static native boolean nativeHasLatestVersion(@NonNull String countryId);

  /**
   * Returns downloading error for given {@code root} node.
   */
  public static native int nativeGetError(String root);

  /**
   * Returns localized name for given {@code root} node.
   */
  public static native String nativeGetName(String root);

  /**
   * Returns country ID corresponding to given coordinates or {@code null} on error.
   */
  public static native @Nullable String nativeFindCountry(double lat, double lon);

  /**
   * Determines whether something is downloading now.
   */
  public static boolean isDownloading()
  {
    return nativeIsDownloading();
  }

  private static native boolean nativeIsDownloading();

  /**
   * Enqueues given {@code root} node and its children in downloader.
   */
  private static native void nativeDownload(String root, boolean withTerrain);

  /**
   * Enqueues failed items under given {@code root} node in downloader.
   */
  private static native void nativeRetry(String root);

  /**
   * Enqueues given {@code root} node with its children in downloader.
   */
  private static native void nativeUpdate(String root);

  /**
   * Removes given currently downloading {@code root} node and its children from downloader.
   */
  public static void cancel(String root)
  {
    cancel(root, true);
  }

  /** Same as {@link #cancel(String)}; withTerrain=false preserves terrain requests only for installed maps. */
  public static void cancel(String root, boolean withTerrain)
  {
    nativeCancel(root, withTerrain);
  }

  private static native void nativeCancel(String root, boolean withTerrain);

  /**
   * Deletes given installed {@code root} node with its children.
   */
  public static native void nativeDelete(String root);

  /** Whether this country is a leaf of the map hierarchy. */
  public static boolean isLeaf(@NonNull String countryId)
  {
    return nativeIsLeaf(countryId);
  }

  private static native boolean nativeIsLeaf(@NonNull String countryId);

  /**
   * Registers {@code callback} of storage status changed. Returns slot ID which should be used to unsubscribe in {@link
   * #nativeUnsubscribe(int)}.
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

  /**
   * Determines if there are unsaved editor changes present for given {@code root}.
   */
  public static native boolean nativeHasUnsavedEditorChanges(String root);

  /**
   * Calculates joint progress of downloading countries specified by {@code countries} array.
   * @return 0 to 100 percent.
   */
  public static int getOverallProgress(String[] countries)
  {
    long[] progress = getOverallProgressBytes(countries);
    return progress[1] == 0 ? 0 : (int) (100.0 * progress[0] / progress[1]);
  }

  /** Returns downloaded and total map bytes. */
  @NonNull
  public static long[] getOverallProgressBytes(String[] countries)
  {
    return nativeGetOverallProgressBytes(countries);
  }

  @NonNull
  private static native long[] nativeGetOverallProgressBytes(String[] countries);

  /** Whether the failed downloads will no longer be retried automatically. */
  public static boolean isAutoRetryFailed()
  {
    return nativeIsAutoretryFailed();
  }

  private static native boolean nativeIsAutoretryFailed();

  /**
   * Returns {@code true} if the core is allowed to download maps while on 3g network. {@code false} otherwise.
   */
  public static native boolean nativeIsDownloadOn3gEnabled();

  /**
   * Sets flag which allows to download maps on 3G.
   */
  public static native void nativeEnableDownloadOn3g();

  /**
   * Returns country ID which the current PP object points to, or {@code null}.
   */
  public static native @Nullable String nativeGetSelectedCountry();
}
