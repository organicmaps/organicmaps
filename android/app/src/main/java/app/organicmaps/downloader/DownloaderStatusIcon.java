package app.organicmaps.downloader;

import android.content.Context;
import android.util.SparseIntArray;
import android.view.View;
import android.widget.ImageView;
import androidx.annotation.AttrRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.StringRes;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.WheelProgressView;

public class DownloaderStatusIcon
{
  private final View mFrame;
  protected final ImageView mIcon;
  private final WheelProgressView mProgress;

  private static final SparseIntArray sIconsCache = new SparseIntArray();

  public DownloaderStatusIcon(View frame)
  {
    mFrame = frame;
    mIcon = mFrame.findViewById(R.id.downloader_status);
    mProgress = mFrame.findViewById(R.id.downloader_progress_wheel);
    DownloadAccessibility.attachButtonRole(mIcon);
    DownloadAccessibility.attachButtonRole(mProgress);
  }

  public DownloaderStatusIcon setOnIconClickListener(View.OnClickListener listener)
  {
    mIcon.setOnClickListener(listener);
    mIcon.setClickable(listener != null);
    mIcon.setFocusable(listener != null);
    return this;
  }

  public DownloaderStatusIcon setOnCancelClickListener(View.OnClickListener listener)
  {
    mProgress.setOnClickListener(listener);
    mProgress.setClickable(listener != null);
    mProgress.setFocusable(listener != null);
    return this;
  }

  protected @AttrRes int selectIcon(CountryItem country)
  {
    return switch (country.status)
    {
      case CountryItem.STATUS_DONE -> R.attr.status_done;
      case CountryItem.STATUS_DOWNLOADABLE, CountryItem.STATUS_PARTLY -> R.attr.status_downloadable;
      case CountryItem.STATUS_FAILED -> R.attr.status_failed;
      case CountryItem.STATUS_UPDATABLE -> R.attr.status_updatable;
      default -> throw new IllegalArgumentException("Inappropriate item status: " + country.status);
    };
  }

  protected @StringRes int getIconAction(CountryItem country)
  {
    return switch (country.status)
    {
      case CountryItem.STATUS_DOWNLOADABLE, CountryItem.STATUS_PARTLY -> R.string.downloader_download_map;
      case CountryItem.STATUS_FAILED -> R.string.downloader_retry;
      case CountryItem.STATUS_UPDATABLE -> R.string.downloader_update_map;
      default -> R.string.downloader_map_actions;
    };
  }

  private @DrawableRes int resolveIcon(@AttrRes int iconAttr)
  {
    int res = sIconsCache.get(iconAttr);
    if (res == 0)
    {
      res = ThemeUtils.getResource(mFrame.getContext(), R.attr.downloaderTheme, iconAttr);
      sIconsCache.put(iconAttr, res);
    }

    return res;
  }

  protected void updateIcon(CountryItem country)
  {
    @AttrRes
    int iconAttr = selectIcon(country);
    @DrawableRes
    int icon = resolveIcon(iconAttr);

    mIcon.setImageResource(icon);
  }

  public void update(CountryItem country)
  {
    boolean pending = (country.status == CountryItem.STATUS_ENQUEUED);
    boolean inProgress =
        (country.status == CountryItem.STATUS_PROGRESS || country.status == CountryItem.STATUS_APPLYING || pending);

    UiUtils.showIf(inProgress, mProgress);
    UiUtils.showIf(!inProgress, mIcon);
    mProgress.setPending(pending);
    final Context context = mFrame.getContext();

    if (inProgress)
    {
      if (!pending)
        mProgress.setProgress(Math.round(country.progress));
      DownloadAccessibility.updateProgress(mProgress, country.name, country.status, country.progress);
      return;
    }

    ViewCompat.setStateDescription(mProgress, null);
    mIcon.setContentDescription(
        context.getString(R.string.downloader_map_action, context.getString(getIconAction(country)), country.name));
    updateIcon(country);
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
  }

  public static void clearCache()
  {
    sIconsCache.clear();
  }
}
