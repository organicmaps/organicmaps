package app.organicmaps.downloader;

import android.app.Activity;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;
import androidx.core.util.Consumer;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.ExpandRetryConfirmationListener;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.ConnectionState;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.lang.ref.WeakReference;

public class MapManagerHelper
{
  private static WeakReference<AlertDialog> sCurrentErrorDialog;

  @StringRes
  public static int getErrorCodeStrRes(final int errorCode)
  {
    return switch (errorCode)
    {
      case CountryItem.ERROR_NO_INTERNET -> R.string.common_check_internet_connection_dialog;
      case CountryItem.ERROR_OOM -> R.string.downloader_no_space_title;
      default -> throw new IllegalArgumentException("Given error can not be displayed: " + errorCode);
    };
  }

  public static void showError(final Activity activity, final MapManager.StorageCallbackData errorData,
                               @Nullable final Consumer<Boolean> dialogClickListener)
  {
    if (!MapManager.nativeIsAutoretryFailed())
      return;

    showErrorDialog(activity, errorData, dialogClickListener);
  }

  public static void showErrorDialog(final Activity activity, final MapManager.StorageCallbackData errorData,
                                     @Nullable final Consumer<Boolean> dialogClickListener)
  {
    if (sCurrentErrorDialog != null)
    {
      AlertDialog dlg = sCurrentErrorDialog.get();
      if (dlg != null && dlg.isShowing())
        return;
    }

    final AlertDialog dlg = new MaterialAlertDialogBuilder(activity, R.style.MwmTheme_AlertDialog)
                                .setTitle(R.string.country_status_download_failed)
                                .setMessage(getErrorCodeStrRes(errorData.errorCode))
                                .setNegativeButton(R.string.cancel,
                                                   (dialog, which) -> {
                                                     sCurrentErrorDialog = null;
                                                     if (dialogClickListener != null)
                                                       dialogClickListener.accept(false);
                                                   })
                                .setPositiveButton(R.string.downloader_retry,
                                                   (dialog, which) -> {
                                                     ExpandRetryConfirmationListener listener =
                                                         new ExpandRetryConfirmationListener(dialogClickListener);
                                                     warn3gAndRetry(activity, errorData.countryId, listener);
                                                   })
                                .create();
    dlg.setCanceledOnTouchOutside(false);
    dlg.show();
    sCurrentErrorDialog = new WeakReference<>(dlg);
  }

  private static void notifyNoSpaceInternal(Activity activity)
  {
    new MaterialAlertDialogBuilder(activity, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.downloader_no_space_title)
        .setMessage(R.string.downloader_no_space_message)
        .setPositiveButton(android.R.string.ok, null)
        .show();
  }

  /**
   * @return true if there is no space to update the given {@code root}, so the alert dialog will be shown.
   */
  private static boolean notifyNoSpaceToUpdate(Activity activity, String root)
  {
    if (MapManager.nativeHasSpaceToUpdate(root))
      return false;

    notifyNoSpaceInternal(activity);
    return true;
  }

  /**
   * @return true if there is no space to download the given {@code root}, so the alert dialog will be shown.
   */
  private static boolean notifyNoSpace(Activity activity, String root)
  {
    if (MapManager.nativeHasSpaceToDownloadCountry(root))
      return false;

    notifyNoSpaceInternal(activity);
    return true;
  }

  /**
   * @return true if there is no space to download {@code size} bytes, so the alert dialog will be shown.
   */
  private static boolean notifyNoSpace(Activity activity, long size)
  {
    if (MapManager.nativeHasSpaceToDownloadAmount(size))
      return false;

    notifyNoSpaceInternal(activity);
    return true;
  }

  private static boolean warnOn3gInternal(Activity activity, @NonNull final Runnable onAcceptListener)
  {
    if (MapManager.nativeIsDownloadOn3gEnabled() || !ConnectionState.INSTANCE.isMobileConnected())
    {
      onAcceptListener.run();
      return false;
    }

    new MaterialAlertDialogBuilder(activity, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.download_over_mobile_header)
        .setMessage(R.string.download_over_mobile_message)
        .setNegativeButton(R.string.cancel, null)
        .setPositiveButton(R.string.ok,
                           (dlg, which) -> {
                             MapManager.nativeEnableDownloadOn3g();
                             onAcceptListener.run();
                           })
        .show();

    return true;
  }

  public static boolean warnOn3gUpdate(Activity activity, @Nullable String countryId,
                                       @NonNull final Runnable onAcceptListener)
  {
    // noinspection SimplifiableIfStatement
    if (TextUtils.isEmpty(countryId) || !notifyNoSpaceToUpdate(activity, countryId))
      return warnOn3gInternal(activity, onAcceptListener);

    return true;
  }

  public static boolean warnOn3g(Activity activity, @Nullable String countryId,
                                 @NonNull final Runnable onAcceptListener)
  {
    // noinspection SimplifiableIfStatement
    if (TextUtils.isEmpty(countryId) || !notifyNoSpace(activity, countryId))
      return warnOn3gInternal(activity, onAcceptListener);

    return true;
  }

  public static boolean warnOn3g(Activity activity, long size, @NonNull Runnable onAcceptListener)
  {
    return !notifyNoSpace(activity, size) && warnOn3gInternal(activity, onAcceptListener);
  }

  public static boolean warn3gAndDownload(Activity activity, final String countryId,
                                          @Nullable final Runnable onAcceptListener)
  {
    return warnOn3g(activity, countryId, () -> {
      if (onAcceptListener != null)
        onAcceptListener.run();
      startDownload(countryId);
    });
  }

  public static boolean warn3gAndRetry(Activity activity, final String countryId,
                                       @Nullable final Runnable onAcceptListener)
  {
    return warnOn3g(activity, countryId, () -> {
      if (onAcceptListener != null)
        onAcceptListener.run();
      retryDownload(countryId);
    });
  }

  /**
   * Enqueues failed items under given {@code root} node in downloader.
   */
  public static void retryDownload(@NonNull String countryId)
  {
    DownloaderService.startForegroundService();
    MapManager.retryDownload(countryId);
  }

  /**
   * Enqueues given {@code root} node with its children in downloader.
   */
  public static void startUpdate(@NonNull String root)
  {
    DownloaderService.startForegroundService();
    MapManager.startUpdate(root);
  }

  /**
   * Enqueues the given list of nodes and its children in downloader.
   */
  public static void startDownload(String... countries)
  {
    DownloaderService.startForegroundService();
    for (var countryId : countries)
    {
      MapManager.startDownload(countryId);
    }
  }

  /**
   * Enqueues given {@code root} node and its children in downloader.
   */
  public static void startDownload(@NonNull String countryId)
  {
    DownloaderService.startForegroundService();
    MapManager.startDownload(countryId);
  }
}
