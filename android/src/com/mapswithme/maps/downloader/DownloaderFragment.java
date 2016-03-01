package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.util.HashMap;
import java.util.Map;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.search.NativeSearchListener;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class DownloaderFragment extends BaseMwmRecyclerFragment
                             implements OnBackPressListener
{
  private DownloaderToolbarController mToolbarController;

  private View mBottomPanel;
  private Button mPanelAction;
  private TextView mPanelText;
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

  private final NativeSearchListener mSearchListener = new NativeSearchListener()
  {
    private final Map<String, CountryItem> mResults = new HashMap<>();

    @Override
    public void onResultsUpdate(SearchResult[] results, long timestamp)
    {
      if (!mSearchRunning || timestamp != mCurrentSearch)
        return;

      for (SearchResult result : results)
      {
        String id = MapManager.nativeFindCountry(result.lat, result.lon);
        if (!TextUtils.isEmpty(id) && !mResults.containsKey(id))
        {
          CountryItem item = CountryItem.fill(id);
          item.category = CountryItem.CATEGORY_ALL;
          mResults.put(id, item);
        }
      }
    }

    @Override
    public void onResultsEnd(long timestamp)
    {
      if (!mSearchRunning || timestamp != mCurrentSearch)
        return;

      mAdapter.setSearchResultsMode(mResults.values());
      mResults.clear();

      onSearchEnd();
    }
  };

  void startSearch()
  {
    mSearchRunning = true;
    mCurrentSearch = System.nanoTime();
    SearchEngine.searchMaps(mToolbarController.getQuery(), mCurrentSearch);
    mToolbarController.showProgress(true);
  }

  void cancelSearch()
  {
    onSearchEnd();
    mAdapter.cancelSearch();
  }

  private void onSearchEnd()
  {
    mSearchRunning = false;
    mToolbarController.showProgress(false);
  }

  void update()
  {
    String rootName = mAdapter.getCurrentParentName();
    boolean onTop = (mAdapter.isSearchResultsMode() || rootName == null);

    // Toolbar
    mToolbarController.update();
    if (!onTop)
      mToolbarController.setTitle(rootName);  // FIXME: Title not shown. Investigate this.

    // Bottom panel
    boolean showBottom = onTop;
    if (showBottom)
    {
      UpdateInfo info = MapManager.nativeGetUpdateInfo();
      showBottom = (info != null && info.filesCount > 0);

      if (showBottom)
        mPanelText.setText(getString(R.string.downloader_maps_to_update, info.filesCount, StringUtils.getFileSizeString(info.totalSize)));
    }

    UiUtils.showIf(showBottom, mBottomPanel);
  }

  @Override
  public void onAttach(Activity activity)
  {
    super.onAttach(activity);
    mSubscriberSlot = MapManager.nativeSubscribe(new MapManager.StorageCallback()
    {
      @Override
      public void onStatusChanged(String countryId, int newStatus, boolean isLeafNode)
      {
        if (isAdded())
          update();
      }

      @Override
      public void onProgress(String countryId, long localSize, long remoteSize) {}
    });

    SearchEngine.INSTANCE.addListener(mSearchListener);
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    if (mSubscriberSlot != 0)
    {
      MapManager.nativeUnsubscribe(mSubscriberSlot);
      mSubscriberSlot = 0;
    }

    SearchEngine.INSTANCE.removeListener(mSearchListener);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    getRecyclerView().addOnScrollListener(mScrollListener);
    mAdapter.attach();

    mBottomPanel = view.findViewById(R.id.bottom_panel);
    mPanelAction = (Button) mBottomPanel.findViewById(R.id.btn__action);
    mPanelText = (TextView) mBottomPanel.findViewById(R.id.tv__text);
    UiUtils.updateButton(mPanelAction);

    mToolbarController = new DownloaderToolbarController(view, getActivity(), this);

    update();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mAdapter.detach();
    mAdapter = null;
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
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
