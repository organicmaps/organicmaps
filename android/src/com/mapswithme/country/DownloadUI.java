package com.mapswithme.country;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ListView;

import com.mapswithme.maps.ContextMenu;
import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.util.ConnectionState;


public class DownloadUI extends MapsWithMeBaseListActivity implements MapStorage.Listener
{
  static String TAG = "DownloadUI";

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.downloader_list_view);

    setListAdapter(new DownloadAdapter(this));
  }

  private DownloadAdapter getDA()
  {
    return (DownloadAdapter) getListView().getAdapter();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    getDA().onResume(this);
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    getDA().onPause();
  }

  @Override
  public void onBackPressed()
  {
    if (getDA().onBackPressed())
    {
      // scroll list view to the top
      setSelection(0);
    }
    else
    {
      super.onBackPressed();
      // Always show map as parent
      startActivity(new Intent(this, MWMActivity.class));
    }
  }

  @Override
  public void onCountryStatusChanged(final Index idx)
  {
    if (getDA().onCountryStatusChanged(idx) == MapStorage.DOWNLOAD_FAILED)
    {
      // Show wireless settings page if no connection found.
      if (ConnectionState.getState(this) == ConnectionState.NOT_CONNECTED)
      {
        final DownloadUI activity = this;
        final String country = getDA().mStorage.countryName(idx);

        runOnUiThread(new Runnable()
        {
          @Override
          public void run()
          {
            new AlertDialog.Builder(activity)
            .setCancelable(false)
            .setMessage(String.format(getString(R.string.download_country_failed), country))
            .setPositiveButton(getString(R.string.connection_settings), new DialogInterface.OnClickListener()
            {
              @Override
              public void onClick(DialogInterface dlg, int which)
              {
                try
                {
                  startActivity(new Intent(Settings.ACTION_WIRELESS_SETTINGS));
                }
                catch (final Exception ex)
                {
                  Log.e(TAG, "Can't run activity:" + ex);
                }

                dlg.dismiss();
              }
            })
            .setNegativeButton(getString(R.string.close), new DialogInterface.OnClickListener()
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
        });
      }
    }
  }

  @Override
  public void onCountryProgress(Index idx, long current, long total)
  {
    getDA().onCountryProgress(getListView(), idx, current, total);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    return ContextMenu.onCreateOptionsMenu(this, menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (ContextMenu.onOptionsItemSelected(this, item))
      return true;
    else
      return super.onOptionsItemSelected(item);
  }
}
