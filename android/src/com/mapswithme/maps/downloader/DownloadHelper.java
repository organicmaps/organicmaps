package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;

import com.mapswithme.maps.R;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Constants;

public class DownloadHelper
{
  private DownloadHelper() {}

  public interface OnDownloadListener
  {
    void onDownload();
  }

  public static boolean canDownloadWithoutWarning(long size)
  {
    return size < 50 * Constants.MB || ConnectionState.isWifiConnected();
  }

  public static void downloadWithCellularCheck(Activity activity, long size, String name, final OnDownloadListener listener)
  {
    if (canDownloadWithoutWarning(size))
      listener.onDownload();
    else
      new AlertDialog.Builder(activity)
          .setCancelable(true)
          .setMessage(String.format(activity.getString(R.string.no_wifi_ask_cellular_download), name))
          .setPositiveButton(activity.getString(R.string.ok), new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              listener.onDownload();
              dlg.dismiss();
            }
          })
          .setNegativeButton(activity.getString(R.string.close), new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dlg, int which)
            {
              dlg.dismiss();
            }
          })
          .create()
          .show();
  }
}
