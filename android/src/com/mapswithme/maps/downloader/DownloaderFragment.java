package com.mapswithme.maps.downloader;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.search.NativeMapSearchListener;
import com.mapswithme.maps.search.SearchEngine;

public class DownloaderFragment extends BaseMwmRecyclerFragment
                             implements OnBackPressListener
{
  private DownloaderToolbarController mToolbarController;

  private BottomPanel mBottomPanel;
  private DownloaderAdapter mAdapter;

  private long mCurrentSearch;
  private boolean mSearchRunning;

  private int mSubscriberSlot;

  private final RecyclerView.OnScrollListener mScrollListener = new RecyclerView.OnScrollListener() {
    @Override
    public void onScrollStateChanged(RecyclerView recyclerView, int newState)
    {
      if (newState == RecyclerView.SCROLL_STATE_DRAGGING)
        mToolbarController.deactivate();
    }
  };

  private final NativeMapSearchListener mSearchListener = new NativeMapSearchListener()
  {
    private final Map<String, CountryItem> mResults = new HashMap<>();

    @Override
    public void onMapSearchResults(Result[] results, long timestamp, boolean isLast)
    {
      if (!mSearchRunning || timestamp != mCurrentSearch)
        return;

      for (Result result : results)
      {
        if (TextUtils.isEmpty(result.countryId) || mResults.containsKey(result.countryId))
          continue;

        CountryItem item = CountryItem.fill(result.countryId);
        item.searchResultName = result.matchedString;
        mResults.put(result.countryId, item);
      }

      if (isLast)
      {
        mAdapter.setSearchResultsMode(mResults.values(), mToolbarController.getQuery());
        mResults.clear();

        onSearchEnd();
      }
    }
  };

  void startSearch()
  {
    mSearchRunning = true;
    mCurrentSearch = System.nanoTime();
    SearchEngine.searchMaps(mToolbarController.getQuery(), mCurrentSearch);
    mToolbarController.showProgress(true);
  }

  void clearSearchQuery()
  {
    mToolbarController.clear();
  }

  void cancelSearch()
  {
    if (!mAdapter.isSearchResultsMode())
      return;

    mAdapter.cancelSearch();
    onSearchEnd();
  }

  private void onSearchEnd()
  {
    mSearchRunning = false;
    mToolbarController.showProgress(false);
    update();
  }

  void update()
  {
    String rootName = mAdapter.getCurrentParentName();
    boolean onTop = (mAdapter.isSearchResultsMode() || rootName == null);

    mToolbarController.update(onTop ? "" : rootName);
    mBottomPanel.update();
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mSubscriberSlot = MapManager.nativeSubscribe(new MapManager.StorageCallback()
    {
      @Override
      public void onStatusChanged(List<MapManager.StorageCallbackData> data)
      {
        if (isAdded())
          update();
      }

      @Override
      public void onProgress(String countryId, long localSize, long remoteSize) {}
    });

    SearchEngine.INSTANCE.addMapListener(mSearchListener);

    getRecyclerView().addOnScrollListener(mScrollListener);
    mAdapter.attach();

    mBottomPanel = new BottomPanel(this, view.findViewById(R.id.bottom_panel));
    mToolbarController = new DownloaderToolbarController(view, getActivity(), this);

    update();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mAdapter.detach();
    mAdapter = null;

    if (mSubscriberSlot != 0)
    {
      MapManager.nativeUnsubscribe(mSubscriberSlot);
      mSubscriberSlot = 0;
    }

    SearchEngine.INSTANCE.removeMapListener(mSearchListener);
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    if (getRecyclerView() != null)
      getRecyclerView().removeOnScrollListener(mScrollListener);
  }

  @Override
  public boolean onBackPressed()
  {
    if (mToolbarController.hasQuery())
    {
      mToolbarController.clear();
      return true;
    }

    return mAdapter.goUpwards();
  }

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_downloader;
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    if (mAdapter == null)
      mAdapter = new DownloaderAdapter(this);

    return mAdapter;
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mToolbarController.onActivityResult(requestCode, resultCode, data);
  }

  @Override
  public DownloaderAdapter getAdapter()
  {
    return mAdapter;
  }
}
