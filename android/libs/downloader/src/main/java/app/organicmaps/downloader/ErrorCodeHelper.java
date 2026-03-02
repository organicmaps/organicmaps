package app.organicmaps.downloader;

import androidx.annotation.StringRes;
import app.organicmaps.sdk.downloader.CountryItem;

public final class ErrorCodeHelper
{
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
}
