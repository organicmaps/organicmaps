package com.mapswithme.country;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.util.ConnectionState;


public class DownloadActivity extends MapsWithMeBaseListActivity implements MapStorage.Listener, View.OnClickListener
{
  static String TAG = DownloadActivity.class.getName();
  private ExtendedDownloadAdapterWrapper mExtendedAdapter;
  private DownloadedAdapter mDownloadedAdapter;
  private TextView mAbButton;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.downloader_list_view);

    mExtendedAdapter = new ExtendedDownloadAdapterWrapper(this, new DownloadAdapter(this));
    setListAdapter(mExtendedAdapter);
  }

  // TODO finish updateall/cancelall functions when screen design'll be ready
//  @Override
//  public boolean onCreateOptionsMenu(Menu menu)
//  {
//    MenuInflater inflater = getMenuInflater();
//    inflater.inflate(R.menu.ab_downloader, menu);
//
//    final MenuItem item = menu.findItem(R.id.item_update);
//    mAbButton = (TextView) item.getActionView();
//    mAbButton.setOnClickListener(this);
//    mAbButton.setVisibility(View.GONE);
//
//    updateActionBar();
//    return true;
//  }

  private BaseDownloadAdapter getDownloadAdapter()
  {
    return (BaseDownloadAdapter) getListView().getAdapter();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    getDownloadAdapter().onResume(getListView());
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    getDownloadAdapter().onPause();
  }

  @Override
  public void onBackPressed()
  {
    if (getDownloadAdapter().onBackPressed())
    {
      // scroll list view to the top
      setSelection(0);
    }
    else if (getListAdapter() instanceof DownloadedAdapter)
    {
      mDownloadedAdapter.onPause();
      mExtendedAdapter.onResume(getListView());
      setListAdapter(mExtendedAdapter);
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
    updateActionBar();

//    if (getDownloadAdapter().onCountryStatusChanged() == MapStorage.DOWNLOAD_FAILED)
//    {
      // Show wireless settings page if no connection found.
      if (!ConnectionState.isConnected(this))
      {
        final DownloadActivity activity = this;
        final String country = MapStorage.INSTANCE.countryName(idx);

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
                    } catch (final Exception ex)
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
//    }
  }

  @Override
  public void onCountryProgress(Index idx, long current, long total)
  {

  }

  private void updateActionBar()
  {
//    // TODO finish updateall/cancelall functions when screen design'll be ready
//    if ()
//    {
//
//    }
//    else
//    if (MapStorage.INSTANCE.getOutdatedCountriesCount() > 0 && mAbButton != null)
//    {
//      mAbButton.setText(getString(R.string.downloader_update_all));
//      mAbButton.setTextColor(getResources().getColor(R.color.downloader_green));
//      mAbButton.setVisibility(View.VISIBLE);
//    }
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    if (getListAdapter().getItemViewType(position) == ExtendedDownloadAdapterWrapper.TYPE_EXTENDED)
    {
      setListAdapter(getDownloadedAdapter());
      mExtendedAdapter.onPause();
      mDownloadedAdapter.onResume(getListView());
    }
  }

  private ListAdapter getDownloadedAdapter()
  {
    if (mDownloadedAdapter == null)
      mDownloadedAdapter = new DownloadedAdapter(this);

    return mDownloadedAdapter;
  }

  @Override
  public void onClick(View v)
  {
//    switch (v.getId())
//    {
//    case R.id.item_update:

//      break;
//    }
  }
}
