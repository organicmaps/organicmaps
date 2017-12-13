package com.mapswithme.maps.downloader;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.WindowManager;

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.search.NativeMapSearchListener;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.widget.PlaceholderView;

public class DownloaderFragment extends BaseMwmRecyclerFragment
                             implements OnBackPressListener
{
  private DownloaderToolbarController mToolbarController;

  private BottomPanel mBottomPanel;
  @Nullable
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
    @Override
    public void onMapSearchResults(Result[] results, long timestamp, boolean isLast)
    {
      if (!mSearchRunning || timestamp != mCurrentSearch)
        return;

      List<CountryItem> rs = new ArrayList<>();
      for (Result result : results)
      {
        CountryItem item = CountryItem.fill(result.countryId);
        item.searchResultName = result.matchedString;
        rs.add(item);
      }

      if (mAdapter != null)
        mAdapter.setSearchResultsMode(rs, mToolbarController.getQuery());

      if (isLast)
        onSearchEnd();
    }
  };

  boolean shouldShowSearch()
  {
    return CountryItem.isRoot(getCurrentRoot());
  }

  void startSearch()
  {
    mSearchRunning = true;
    mCurrentSearch = System.nanoTime();
    SearchEngine.searchMaps(mToolbarController.getQuery(), mCurrentSearch);
    mToolbarController.showProgress(true);
    if (mAdapter != null)
      mAdapter.clearAdsAndCancelMyTarget();
  }

  void clearSearchQuery()
  {
    mToolbarController.clear();
  }

  void cancelSearch()
  {
    if (mAdapter == null || !mAdapter.isSearchResultsMode())
      return;

    mAdapter.resetSearchResultsMode();
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
    mToolbarController.update();
    mBottomPanel.update();
  }

  @CallSuper
  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    getActivity().getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
  }

  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
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
    if (mAdapter != null)
    {
      mAdapter.refreshData();
      mAdapter.attach();
    }

    mBottomPanel = new BottomPanel(this, view);
    mToolbarController = new DownloaderToolbarController(view, getActivity(), this);

    update();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    if (mAdapter != null)
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

    return mAdapter != null && mAdapter.goUpwards();
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
  @Nullable
  public DownloaderAdapter getAdapter()
  {
    return mAdapter;
  }

  @NonNull
  String getCurrentRoot()
  {
    return mAdapter != null ? mAdapter.getCurrentRootId() : "";
  }

  @Override
  protected void setupPlaceholder(@NonNull PlaceholderView placeholder)
  {
    if (mAdapter != null && mAdapter.isSearchResultsMode())
      placeholder.setContent(R.drawable.img_mappyny,
                             R.string.search_not_found, R.string.search_not_found_query);
    else
      placeholder.setContent(R.drawable.img_search_no_maps,
                             R.string.downloader_no_downloaded_maps_title, R.string.downloader_no_downloaded_maps_message);
  }
}
