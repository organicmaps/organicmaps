package com.mapswithme.maps.downloader;

import android.location.Location;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class CountrySuggestFragment extends BaseMwmFragment implements View.OnClickListener
{
  private LinearLayout mLlWithLocation;
  private LinearLayout mLlNoLocation;
  private LinearLayout mLlSelectDownload;
  private LinearLayout mLlActiveDownload;
  private WheelProgressView mWpvDownloadProgress;
  private TextView mTvCountry;
  private TextView mTvActiveCountry;
  private Button mBtnDownloadMap;

  private CountryItem mCurrentCountry;
  private CountryItem mDownloadingCountry;
  private int mListenerSlot;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_suggest_country_download, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initViews(view);
    mListenerSlot = MapManager.nativeSubscribe(new MapManager.StorageCallback()
    {
      @Override
      public void onStatusChanged(String countryId, int newStatus)
      {
        if (!isAdded())
          return;

        refreshViews();

        switch (newStatus)
        {
        case CountryItem.STATUS_FAILED:
          UiUtils.checkConnectionAndShowAlert(getActivity(), getString(R.string.download_country_failed, mDownloadingCountry.name));
          break;

        case CountryItem.STATUS_DONE:
          exitFragment();
          break;
        }
      }

      @Override
      public void onProgress(String countryId, long localSize, long remoteSize)
      {
        if (!isAdded())
          return;

        if (mDownloadingCountry == null)
          mDownloadingCountry = CountryItem.fill(countryId);

        refreshViews();

        int percent = (int)(localSize * 100 / remoteSize);
        mWpvDownloadProgress.setProgress(percent);
      }
    });
  }

  private void exitFragment()
  {
    // TODO find more elegant way
    getParentFragment().getChildFragmentManager().beginTransaction().remove(this).commitAllowingStateLoss();
  }

  @Override
  public void onResume()
  {
    super.onResume();

    Location loc = LocationHelper.INSTANCE.getLastLocation();
    if (loc != null)
    {
      String id = MapManager.nativeFindCountry(loc.getLatitude(), loc.getLongitude());
      if (!TextUtils.isEmpty(id))
        mCurrentCountry = CountryItem.fill(id);
    }

    refreshViews();
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    MapManager.nativeUnsubscribe(mListenerSlot);
  }

  private void refreshCountryName()
  {
    if (mDownloadingCountry == null || !isAdded())
      return;

    mTvCountry.setText(mDownloadingCountry.name);
    mTvActiveCountry.setText(mDownloadingCountry.name);
  }

  private void refreshCountrySize()
  {
    if (mCurrentCountry == null || !isAdded())
      return;

    mBtnDownloadMap.setText(getString(R.string.downloader_download_map) +
                            " (" + StringUtils.getFileSizeString(mCurrentCountry.totalSize) + ")");
  }

  private void initViews(View view)
  {
    mLlSelectDownload = (LinearLayout) view.findViewById(R.id.ll__select_download);
    mLlActiveDownload = (LinearLayout) view.findViewById(R.id.ll__active_download);
    mLlWithLocation = (LinearLayout) view.findViewById(R.id.ll__location_determined);
    mLlNoLocation = (LinearLayout) view.findViewById(R.id.ll__location_unknown);
    mBtnDownloadMap = (Button) view.findViewById(R.id.btn__download_map);
    mBtnDownloadMap.setOnClickListener(this);
    Button selectMap = (Button)view.findViewById(R.id.btn__select_map);
    selectMap.setOnClickListener(this);
    mWpvDownloadProgress = (WheelProgressView) view.findViewById(R.id.wpv__download_progress);
    mWpvDownloadProgress.setCenterDrawable(ContextCompat.getDrawable(getActivity(), R.drawable.ic_close));
    mWpvDownloadProgress.setOnClickListener(this);
    mTvCountry = (TextView) view.findViewById(R.id.tv__country_name);
    mTvActiveCountry = (TextView) view.findViewById(R.id.tv__active_country_name);

    UiUtils.updateAccentButton(mBtnDownloadMap);
    UiUtils.updateAccentButton(selectMap);
  }

  private void refreshViews()
  {
    if (!isAdded() || MapManager.nativeGetDownloadedCount() > 0)
      return;

    boolean downloading = MapManager.nativeIsDownloading();
    UiUtils.showIf(downloading, mLlActiveDownload);
    UiUtils.showIf(!downloading, mLlSelectDownload);

    if (downloading)
    {
      refreshCountryName();
      return;
    }

    boolean hasLocation = (mCurrentCountry != null);
    UiUtils.showIf(hasLocation, mLlWithLocation);
    UiUtils.showIf(!hasLocation, mLlNoLocation);

    refreshCountryName();
    refreshCountrySize();
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn__download_map:
      MapManager.nativeDownload(mCurrentCountry.id);
      break;

    case R.id.btn__select_map:
      getMwmActivity().replaceFragment(DownloaderFragment.class, null, null);
      break;

    case R.id.wpv__download_progress:
      MapManager.nativeCancel(mDownloadingCountry.id);
      break;
    }
  }
}
