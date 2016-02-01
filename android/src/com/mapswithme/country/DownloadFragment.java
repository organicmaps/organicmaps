package com.mapswithme.country;

import android.database.DataSetObserver;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.Config;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public class DownloadFragment extends BaseMwmListFragment implements View.OnClickListener, ActiveCountryTree.ActiveCountryListener, OnBackPressListener
{
  private DownloadAdapter mDownloadAdapter;
  private DownloadedAdapter mDownloadedAdapter;
  private TextView mTvUpdateAll;
  private View mDownloadAll;
  private int mMode = MODE_NONE;
  private int mListenerSlotId;
  private LayoutInflater mLayoutInflater;

  private static final int MODE_NONE = 0;
  private static final int MODE_UPDATE_ALL = 1;
  private static final int MODE_CANCEL_ALL = 2;

  private static int getTheme()
  {
    String theme = Config.getCurrentUiTheme();
    if (ThemeUtils.isDefaultTheme(theme))
      return R.style.MwmTheme_Downloader;

    if (ThemeUtils.isNightTheme(theme))
      return R.style.MwmTheme_Night_Downloader;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  LayoutInflater getLayoutInflater()
  {
    if (mLayoutInflater == null)
      mLayoutInflater = ThemeUtils.themedInflater(getActivity().getLayoutInflater(), getTheme());

    return mLayoutInflater;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return getLayoutInflater().inflate(R.layout.fragment_downloader, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initToolbar();
    if (getArguments() != null && getArguments().getBoolean(DownloadActivity.EXTRA_OPEN_DOWNLOADED_LIST, false))
      openDownloadedList();
    else
    {
      mDownloadAdapter = getDownloadAdapter();
      setListAdapter(mDownloadAdapter);
      mMode = MODE_NONE;
      mListenerSlotId = ActiveCountryTree.addListener(this);
    }
  }

  private void initToolbar()
  {
    final Toolbar toolbar = getToolbar();
    toolbar.setTitle(getString(R.string.maps));
    toolbar.setNavigationOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onBackPressed();
      }
    });

    mTvUpdateAll = (TextView) toolbar.findViewById(R.id.tv__update_all);
    mTvUpdateAll.setOnClickListener(this);

    mDownloadAll = toolbar.findViewById(R.id.download_all);
    mDownloadAll.setOnClickListener(this);
    UiUtils.hide(mTvUpdateAll, mDownloadAll);
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();

    ActiveCountryTree.removeListener(mListenerSlotId);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    getDownloadedAdapter().setCountryListener();
    getDownloadAdapter().onResume(getListView());
  }

  @Override
  public void onPause()
  {
    super.onPause();
    getDownloadedAdapter().resetCountryListener();
    getDownloadAdapter().onPause();
  }

  @Override
  public boolean onBackPressed()
  {
    if (getDownloadAdapter().onBackPressed())
    {
      setSelection(0);
      updateToolbar();
    }
    else if (getListAdapter() instanceof DownloadedAdapter)
    {
      mMode = MODE_NONE;
      mDownloadedAdapter.onPause();
      mDownloadAdapter = getDownloadAdapter();
      mDownloadAdapter.onResume(getListView());
      setListAdapter(mDownloadAdapter);
      updateToolbar();
    }
    else
      Utils.navigateToParent(getActivity());

    return true;
  }

  private void updateToolbar()
  {
    if (mTvUpdateAll == null || !isAdded())
      return;

    updateMode();
    if (mMode == MODE_NONE)
    {
      mTvUpdateAll.setVisibility(View.GONE);
      UiUtils.showIf(CountryTree.hasParent() && CountryTree.isDownloadableGroup(), mDownloadAll);
      return;
    }

    UiUtils.hide(mDownloadAll);

    switch (mMode)
    {
    case MODE_CANCEL_ALL:
      mTvUpdateAll.setText(getString(R.string.downloader_cancel_all));
      mTvUpdateAll.setVisibility(View.VISIBLE);
      break;
    case MODE_UPDATE_ALL:
      mTvUpdateAll.setText(getString(R.string.downloader_update_all));
      mTvUpdateAll.setVisibility(View.VISIBLE);
      break;
    }
  }

  private void updateMode()
  {
    if (ActiveCountryTree.isDownloadingActive())
      mMode = MODE_CANCEL_ALL;
    else if (ActiveCountryTree.getOutOfDateCount() > 0)
      mMode = MODE_UPDATE_ALL;
    else
      mMode = MODE_NONE;
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    if (getListAdapter().getItemViewType(position) == DownloadAdapter.TYPE_EXTENDED)
      openDownloadedList();
  }

  private void openDownloadedList()
  {
    setListAdapter(getDownloadedAdapter());
    mMode = MODE_NONE;
    updateToolbar();
    if (mDownloadAdapter != null)
      mDownloadAdapter.onPause();
    mDownloadedAdapter.onResume(getListView());
  }

  BaseDownloadAdapter getDownloadedAdapter()
  {
    if (mDownloadedAdapter == null)
    {
      mDownloadedAdapter = new DownloadedAdapter(this);
      mDownloadedAdapter.registerDataSetObserver(new DataSetObserver()
      {
        @Override
        public void onChanged()
        {
          if (isAdded())
            updateToolbar();
        }
      });
    }

    return mDownloadedAdapter;
  }

  private DownloadAdapter getDownloadAdapter()
  {
    if (mDownloadAdapter == null)
    {
      mDownloadAdapter = new DownloadAdapter(this);
      mDownloadAdapter.registerDataSetObserver(new DataSetObserver()
      {
        @Override
        public void onChanged()
        {
          if (isAdded())
            updateToolbar();
        }
      });
    }

    return mDownloadAdapter;
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.tv__update_all:
      if (mMode == MODE_UPDATE_ALL)
        ActiveCountryTree.updateAll();
      else
        ActiveCountryTree.cancelAll();
      updateToolbar();
      break;

    case R.id.download_all:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MAP_DOWNLOAD_ALL);
      CountryTree.downloadGroup();
      getDownloadAdapter().notifyDataSetInvalidated();
      break;
    }
  }

  @Override
  public void onCountryProgressChanged(int group, int position, long[] sizes) { }

  @Override
  public void onCountryStatusChanged(int group, int position, int oldStatus, int newStatus)
  {
    if (isAdded())
    {
      updateToolbar();
      Utils.keepScreenOn(ActiveCountryTree.isDownloadingActive(), getActivity().getWindow());
    }
  }

  @Override
  public void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition) {}

  @Override
  public void onCountryOptionsChanged(int group, int position, int newOptions, int requestOptions)
  {
    if (isAdded())
      updateToolbar();
  }
}
