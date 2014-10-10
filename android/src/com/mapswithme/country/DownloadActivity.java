package com.mapswithme.country;

import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.MapStorage.Index;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MapsWithMeBaseListActivity;


public class DownloadActivity extends MapsWithMeBaseListActivity implements MapStorage.Listener, View.OnClickListener
{
  static String TAG = DownloadActivity.class.getName();
  private ExtendedDownloadAdapterWrapper mExtendedAdapter;
  private DownloadedAdapter mDownloadedAdapter;
  private TextView mAbButton;
  private int mMode = MODE_DISABLED;

  private static final int MODE_DISABLED = -1;
  private static final int MODE_NONE = 0;
  private static final int MODE_UPDATE_ALL = 1;
  private static final int MODE_CANCEL_ALL = 2;


  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.downloader_list_view);

    mExtendedAdapter = new ExtendedDownloadAdapterWrapper(this, new DownloadAdapter(this));
    setListAdapter(mExtendedAdapter);
    mMode = MODE_DISABLED;
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.ab_downloader, menu);

    final MenuItem item = menu.findItem(R.id.item_update);
    mAbButton = (TextView) item.getActionView();
    mAbButton.setOnClickListener(this);
    mAbButton.setVisibility(View.GONE);

    updateActionBar();
    return true;
  }

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
      mMode = MODE_DISABLED;
      mDownloadedAdapter.onPause();
      mExtendedAdapter.onResume(getListView());
      setListAdapter(mExtendedAdapter);
      updateActionBar();
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
  }

  @Override
  public void onCountryProgress(Index idx, long current, long total)
  {

  }

  private void updateActionBar()
  {
    if (mAbButton == null)
      return;
    if (mMode == MODE_DISABLED)
    {
      mAbButton.setVisibility(View.GONE);
      return;
    }
    if (ActiveCountryTree.isDownloadingActive())
    {
      mMode = MODE_CANCEL_ALL;
      // TODO text - stop all
      mAbButton.setText(getString(R.string.cancel));
      mAbButton.setTextColor(getResources().getColor(R.color.downloader_red));
      mAbButton.setVisibility(View.VISIBLE);
    }
    else if (ActiveCountryTree.getCountInGroup(ActiveCountryTree.GROUP_OUT_OF_DATE) > 0)
    {
      mMode = MODE_UPDATE_ALL;
      mAbButton.setText(getString(R.string.downloader_update_all));
      mAbButton.setTextColor(getResources().getColor(R.color.downloader_green));
      mAbButton.setVisibility(View.VISIBLE);
    }
    invalidateOptionsMenu();
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id)
  {
    if (getListAdapter().getItemViewType(position) == ExtendedDownloadAdapterWrapper.TYPE_EXTENDED)
    {
      setListAdapter(getDownloadedAdapter());
      mMode = MODE_NONE;
      updateActionBar();
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
    switch (v.getId())
    {
    case R.id.item_update:
      if (mMode == MODE_UPDATE_ALL)
        ActiveCountryTree.updateAll();
      else
        ActiveCountryTree.cancelAll();
      updateActionBar();
      break;
    }
  }
}
