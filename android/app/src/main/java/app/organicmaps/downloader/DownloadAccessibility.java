package app.organicmaps.downloader;

import android.content.Context;
import android.view.View;
import android.widget.Button;
import androidx.core.os.ConfigurationCompat;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import java.text.NumberFormat;
import java.util.Locale;

final class DownloadAccessibility
{
  static void attachButtonRole(View view)
  {
    ViewCompat.setAccessibilityDelegate(view, new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        if (host.hasOnClickListeners())
          info.setClassName(Button.class.getName());
      }
    });
  }

  static void updateProgress(View view, String country, int status, float progress)
  {
    Context context = view.getContext();
    view.setContentDescription(
        context.getString(R.string.downloader_map_action, context.getString(R.string.cancel_download), country));
    String state;
    if (status == CountryItem.STATUS_ENQUEUED)
      state = context.getString(R.string.downloader_queued);
    else
    {
      Locale locale = ConfigurationCompat.getLocales(context.getResources().getConfiguration()).get(0);
      if (locale == null)
        locale = Locale.getDefault();
      String percent = NumberFormat.getPercentInstance(locale).format(Math.round(progress) / 100.0);
      state = context.getString(status == CountryItem.STATUS_APPLYING ? R.string.downloader_applying_percent
                                                                      : R.string.downloader_progress_percent,
                                percent);
    }
    ViewCompat.setStateDescription(view, state);
  }

  private DownloadAccessibility() {}
}
