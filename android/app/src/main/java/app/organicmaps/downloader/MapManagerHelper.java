package app.organicmaps.downloader;

import android.content.Context;
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

  public static void showError(final Context context, final MapManager.StorageCallbackData errorData,
                               @Nullable final Consumer<Boolean> dialogClickListener)
  {
    if (!MapManager.nativeIsAutoretryFailed())
      return;

    showErrorDialog(context, errorData, dialogClickListener);
  }

  public static void showErrorDialog(final Context context, final MapManager.StorageCallbackData errorData,
                                     @Nullable final Consumer<Boolean> dialogClickListener)
  {
    if (sCurrentErrorDialog != null)
    {
      AlertDialog dlg = sCurrentErrorDialog.get();
      if (dlg != null && dlg.isShowing())
        return;
    }

    final AlertDialog dlg = new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
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
                                                     warn3gAndRetry(context, errorData.countryId, listener);
                                                   })
                                .create();
    dlg.setCanceledOnTouchOutside(false);
    dlg.show();
    sCurrentErrorDialog = new WeakReference<>(dlg);
  }

  private static void notifyNoSpaceInternal(Context context)
  {
    new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.downloader_no_space_title)
        .setMessage(R.string.downloader_no_space_message)
        .setPositiveButton(android.R.string.ok, null)
        .show();
  }

  /**
   * @return true if there is no space to update the given {@code root}, so the alert dialog will be shown.
   */
  private static boolean notifyNoSpaceToUpdate(Context context, String root)
  {
    if (MapManager.nativeHasSpaceToUpdate(root))
      return false;

    notifyNoSpaceInternal(context);
    return true;
  }

  /**
   * @return true if there is no space to download the given {@code root}, so the alert dialog will be shown.
   */
  private static boolean notifyNoSpace(Context context, String root)
  {
    if (MapManager.nativeHasSpaceToDownloadCountry(root))
      return false;

    notifyNoSpaceInternal(context);
    return true;
  }

  /**
   * @return true if there is no space to download {@code size} bytes, so the alert dialog will be shown.
   */
  private static boolean notifyNoSpace(Context context, long size)
  {
    if (MapManager.nativeHasSpaceToDownloadAmount(size))
      return false;

    notifyNoSpaceInternal(context);
    return true;
  }

  private static boolean warnOn3gInternal(Context context, @NonNull final Runnable onAcceptListener)
  {
    if (MapManager.nativeIsDownloadOn3gEnabled() || !ConnectionState.INSTANCE.isMobileConnected())
    {
      onAcceptListener.run();
      return false;
    }

    new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
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

  public static boolean warnOn3gUpdate(Context context, @Nullable String countryId,
                                       @NonNull final Runnable onAcceptListener)
  {
    // noinspection SimplifiableIfStatement
    if (TextUtils.isEmpty(countryId) || !notifyNoSpaceToUpdate(context, countryId))
      return warnOn3gInternal(context, onAcceptListener);

    return true;
  }

  public static boolean warnOn3g(Context context, @Nullable String countryId, @NonNull final Runnable onAcceptListener)
  {
    // noinspection SimplifiableIfStatement
    if (TextUtils.isEmpty(countryId) || !notifyNoSpace(context, countryId))
      return warnOn3gInternal(context, onAcceptListener);

    return true;
  }

  public static boolean warnOn3g(Context context, long size, @NonNull Runnable onAcceptListener)
  {
    return !notifyNoSpace(context, size) && warnOn3gInternal(context, onAcceptListener);
  }

  public static boolean warn3gAndDownload(Context context, final String countryId,
                                          @Nullable final Runnable onAcceptListener)
  {
    return warnOn3g(context, countryId, () -> {
      if (onAcceptListener != null)
        onAcceptListener.run();
      startDownload(context, countryId);
    });
  }

  public static boolean warn3gAndRetry(Context context, final String countryId,
                                       @Nullable final Runnable onAcceptListener)
  {
    return warnOn3g(context, countryId, () -> {
      if (onAcceptListener != null)
        onAcceptListener.run();
      retryDownload(context, countryId);
    });
  }

  /**
   * Enqueues failed items under given {@code root} node in downloader.
   */
  public static void retryDownload(Context context, @NonNull String countryId)
  {
    DownloaderService.startForegroundService(context);
    MapManager.retryDownload(countryId);
  }

  /**
   * Enqueues given {@code root} node with its children in downloader.
   */
  public static void startUpdate(Context context, @NonNull String root)
  {
    DownloaderService.startForegroundService(context);
    MapManager.startUpdate(root);
  }

  /**
   * Enqueues the given list of nodes and its children in downloader.
   */
  public static void startDownload(Context context, String... countries)
  {
    DownloaderService.startForegroundService(context);
    for (var countryId : countries)
    {
      MapManager.startDownload(countryId);
    }
  }

  /**
   * Enqueues given {@code root} node and its children in downloader.
   */
  public static void startDownload(Context context, @NonNull String countryId)
  {
    DownloaderService.startForegroundService(context);
    MapManager.startDownload(countryId);
  }
}
