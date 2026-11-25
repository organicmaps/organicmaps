package app.organicmaps.downloader;

import android.content.Context;
import android.util.SparseIntArray;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import androidx.annotation.AttrRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.WheelProgressView;
import java.text.NumberFormat;

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
    final AccessibilityDelegateCompat buttonRole = new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(@NonNull View host, @NonNull AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        if (host.hasOnClickListeners())
          info.setClassName(Button.class.getName());
      }
    };
    ViewCompat.setAccessibilityDelegate(mIcon, buttonRole);
    ViewCompat.setAccessibilityDelegate(mProgress, buttonRole);
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
      mProgress.setContentDescription(
          context.getString(R.string.downloader_map_action, context.getString(R.string.cancel_download), country.name));
      final String state;
      if (pending)
        state = context.getString(R.string.downloader_queued);
      else
      {
        final int progress = Math.round(country.progress);
        mProgress.setProgress(progress);
        final String percent = NumberFormat.getPercentInstance().format(progress / 100.0);
        state = context.getString(country.status == CountryItem.STATUS_APPLYING ? R.string.downloader_applying_percent
                                                                                : R.string.downloader_progress_percent,
                                  percent);
      }
      ViewCompat.setStateDescription(mProgress, state);
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
