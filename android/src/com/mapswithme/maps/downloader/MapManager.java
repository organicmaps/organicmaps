package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.annotation.UiThread;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;

import java.lang.ref.WeakReference;
import java.util.List;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.statistics.Statistics;

@UiThread
public final class MapManager
{
  @SuppressWarnings("unused")
  public static class StorageCallbackData
  {
    public final String countryId;
    public final int newStatus;
    public final int errorCode;
    public final boolean isLeafNode;

    public StorageCallbackData(String countryId, int newStatus, int errorCode, boolean isLeafNode)
    {
      this.countryId = countryId;
      this.newStatus = newStatus;
      this.errorCode = errorCode;
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

  private static WeakReference<AlertDialog> sCurrentErrorDialog;
  private static boolean sSkip3gCheck;

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

  public static void showError(final Activity activity, final StorageCallbackData errorData)
  {
    if (sCurrentErrorDialog != null)
    {
      AlertDialog dlg = sCurrentErrorDialog.get();
      if (dlg != null && dlg.isShowing())
        return;

      sCurrentErrorDialog = null;
    }

    @StringRes int text;
    switch (errorData.errorCode)
    {
    case CountryItem.ERROR_NO_INTERNET:
      text = R.string.common_check_internet_connection_dialog;
      break;

    case CountryItem.ERROR_OOM:
      text = R.string.downloader_no_space_title;
      break;

    default:
      throw new IllegalArgumentException("Give error can not be displayed: " + errorData.errorCode);
    }

    AlertDialog dlg = new AlertDialog.Builder(activity)
                                     .setTitle(R.string.country_status_download_failed)
                                     .setMessage(text)
                                     .setNegativeButton(android.R.string.cancel, null)
                                     .setPositiveButton(R.string.downloader_retry, new DialogInterface.OnClickListener()
                                     {
                                       @Override
                                       public void onClick(DialogInterface dialog, int which)
                                       {
                                         warn3gAndRetry(activity, errorData.countryId, new Runnable()
                                         {
                                           @Override
                                           public void run()
                                           {
                                             Notifier.cancelDownloadFailed();
                                           }
                                         });
                                       }
                                     }).setOnDismissListener(new DialogInterface.OnDismissListener()
                                     {
                                       @Override
                                       public void onDismiss(DialogInterface dialog)
                                       {
                                         sCurrentErrorDialog = null;
                                       }
                                     }).create();
    dlg.show();
    sCurrentErrorDialog = new WeakReference<>(dlg);
  }

  public static void checkUpdates()
  {
    if (!Framework.nativeIsDataVersionChanged())
      return;

    String countriesToUpdate = Framework.nativeGetOutdatedCountriesString();
    if (!TextUtils.isEmpty(countriesToUpdate))
      Notifier.notifyUpdateAvailable(countriesToUpdate);

    Framework.nativeUpdateSavedDataVersion();
  }

  public static boolean warnDownloadOn3g(Activity activity, @NonNull final Runnable onAcceptListener)
  {
    if (sSkip3gCheck || !ConnectionState.isMobileConnected())
    {
      onAcceptListener.run();
      return false;
    }

    new AlertDialog.Builder(activity)
        .setMessage(String.format("%1$s\n\n%2$s", activity.getString(R.string.download_over_mobile_header),
                                                  activity.getString(R.string.download_over_mobile_message)))
        .setNegativeButton(android.R.string.no, null)
        .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            sSkip3gCheck = true;
            onAcceptListener.run();
          }
        }).show();

    return true;
  }

  public static boolean warn3gAndDownload(Activity activity, final String countryId, @Nullable final Runnable onAcceptListener)
  {
    return warnDownloadOn3g(activity, new Runnable()
    {
      @Override
      public void run()
      {
        if (onAcceptListener != null)
          onAcceptListener.run();
        nativeDownload(countryId);
      }
    });
  }

  public static boolean warn3gAndRetry(Activity activity, final String countryId, @Nullable final Runnable onAcceptListener)
  {
    return warnDownloadOn3g(activity, new Runnable()
    {
      @Override
      public void run()
      {
        if (onAcceptListener != null)
          onAcceptListener.run();
        nativeRetry(countryId);
      }
    });
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
   * Returns {@code true} if there is enough storage space to perform migration. Or {@code false} otherwise.
   */
  public static native boolean nativeHasSpaceForMigration();

  /**
   * Determines whether the legacy (large MWMs) mode is used.
   */
  public static native boolean nativeIsLegacyMode();

  /**
   * Quickly determines if the migration is needed. In the most cases you should use {@link #nativeIsLegacyMode()} instead.
   */
  public static native boolean nativeNeedMigrate();

  /**
   * Performs migration from old (large MWMs) mode.
   * @return Name of the country to be loaded during the prefetch.
   *         Or {@code null} if maps were queued to downloader and migration process is complete.
   *         In the latter case {@link MigrationListener#onComplete()} will be called before return from {@code nativeMigrate()}.
   */
  public static native @Nullable String nativeMigrate(MigrationListener listener, double lat, double lon, boolean hasLocation, boolean keepOldMaps);

  /**
   * Aborts migration. Affects only prefetch process.
   */
  public static native void nativeCancelMigration();

  /**
   * Return count of fully downloaded maps (excluding fake MWMs).
   */
  public static native int nativeGetDownloadedCount();

  /**
   * Returns info about updatable data under given {@code root} or null on error.
   */
  public static native @Nullable UpdateInfo nativeGetUpdateInfo(@Nullable String root);

  /**
   * Retrieves list of country items with its status info. Uses root as parent if {@code root} is null.
   */
  public static native void nativeListItems(@Nullable String root, double lat, double lon, boolean hasLocation, List<CountryItem> result);

  /**
   * Sets following attributes of the given {@code item}:
   * <pre>
   * <ul>
   *   <li>name;</li>
   *   <li>directParentId;</li>
   *   <li>topmostParentId;</li>
   *   <li>directParentName;</li>
   *   <li>topmostParentName;</li>
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

  /**
   * Determines if there are unsaved editor changes present for given {@code root}.
   */
  public static native boolean nativeHasUnsavedEditorChanges(String root);
}
