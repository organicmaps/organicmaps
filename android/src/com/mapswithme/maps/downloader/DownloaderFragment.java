package com.mapswithme.maps.downloader;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.search.NativeSearchListener;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class DownloaderFragment extends BaseMwmRecyclerFragment
                             implements OnBackPressListener
{
  private ToolbarController mToolbarController;

  private View mBottomPanel;
  private Button mPanelAction;
  private TextView mPanelText;
  private DownloaderAdapter mAdapter;

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
    @Override
    public void onResultsUpdate(SearchResult[] results, long timestamp)
    {

    }

    @Override
    public void onResultsEnd(long timestamp)
    {

    }
  };

  private class ToolbarController extends SearchToolbarController
  {
    private final View mDownloadAll;
    private final View mUpdateAll;
    private final View mCancelAll;

    ToolbarController(View root, Activity activity)
    {
      super(root, activity);

      mDownloadAll = mContainer.findViewById(R.id.download_all);
      mUpdateAll = mContainer.findViewById(R.id.update_all);
      mCancelAll = mContainer.findViewById(R.id.cancel_all);

      mDownloadAll.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          MapManager.nativeDownload(mAdapter.getCurrentParent());
        }
      });

      mUpdateAll.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          MapManager.nativeUpdate(mAdapter.getCurrentParent());
        }
      });

      mCancelAll.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          MapManager.nativeCancel(mAdapter.getCurrentParent());
        }
      });

      update();
    }

    @Override
    public void onUpClick()
    {
      if (!onBackPressed())
        super.onUpClick();
    }

    public void update()
    {
      boolean cancel = MapManager.nativeIsDownloading();

      boolean update = !cancel;
      if (update)
      {
        // TODO (trashkalmar): Use appropriate function
        update = false;
      }

      boolean onTop = !mAdapter.canGoUpdwards();

      boolean download = (!cancel && !update && !onTop);
      if (download)
      {
        // TODO (trashkalmar): Use appropriate function
        download = true;
      }

      UiUtils.showIf(cancel, mCancelAll);
      UiUtils.showIf(update, mUpdateAll);
      UiUtils.showIf(download, mDownloadAll);
      UiUtils.showIf(onTop, mQuery);
    }

    @Override
    protected void startVoiceRecognition(Intent intent, int code)
    {
      startActivityForResult(intent, code);
    }
  }

  private void updateBottomPanel()
  {
    UpdateInfo info = MapManager.nativeGetUpdateInfo();
    boolean show = (info != null && info.filesCount > 0);
    UiUtils.showIf(show, mBottomPanel);
    if (show)
      mPanelText.setText(getString(R.string.downloader_maps_to_update, info.filesCount, StringUtils.getFileSizeString(info.totalSize)));
  }

  @Override
  public void onActivityCreated(@Nullable Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);

    if (MapManager.nativeIsLegacyMode())
      getMwmActivity().replaceFragment(MigrateSmallMwmFragment.class, null, null);
  }

  @Override
  public void onAttach(Activity activity)
  {
    super.onAttach(activity);
    mSubscriberSlot = MapManager.nativeSubscribe(new MapManager.StorageCallback()
    {
      @Override
      public void onStatusChanged(String countryId, int newStatus)
      {
        if (!isAdded())
          return;

        mToolbarController.update();
        updateBottomPanel();
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
    SearchEngine.cancelSearch();
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

    mToolbarController = new ToolbarController(view, getActivity());
    updateBottomPanel();
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
    {
      mAdapter = new DownloaderAdapter(getRecyclerView(), getActivity());
      mAdapter.registerAdapterDataObserver(new RecyclerView.AdapterDataObserver()
      {
        @Override
        public void onChanged()
        {
          boolean root = TextUtils.isEmpty(mAdapter.getCurrentParent());
          mToolbarController.show(root);
          if (!root)
            mToolbarController.setTitle(R.string.download_maps);
        }
      });
    }

    return mAdapter;
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mToolbarController.onActivityResult(requestCode, resultCode, data);
  }
}
