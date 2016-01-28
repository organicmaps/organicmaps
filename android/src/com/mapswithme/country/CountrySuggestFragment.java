package com.mapswithme.country;

import android.location.Location;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class CountrySuggestFragment extends BaseMwmFragment implements View.OnClickListener, CompoundButton.OnCheckedChangeListener
{
  private double mLat;
  private double mLon;
  private MapStorage.Index mCurrentLocationCountryIndex;
  private int mCountryListenerId;
  private CountryItem mDownloadingCountry;
  private int mDownloadingPosition;
  private int mDownloadingGroup;

  private LinearLayout mLlWithLocation;
  private LinearLayout mLlNoLocation;
  private LinearLayout mLlSelectDownload;
  private LinearLayout mLlActiveDownload;
  private WheelProgressView mWpvDownloadProgress;
  private TextView mTvCountry;
  private TextView mTvActiveCountry;
  private CheckBox mChbRoutes;
  private Button mBtnDownloadMap;

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
    mCountryListenerId = ActiveCountryTree.addListener(new ActiveCountryTree.SimpleCountryTreeListener()
    {
      @Override
      public void onCountryProgressChanged(int group, int position, long[] sizes)
      {
        if (mDownloadingCountry == null)
        {
          mDownloadingCountry = ActiveCountryTree.getCountryItem(group, position);
          mDownloadingPosition = position;
          mDownloadingGroup = position;
        }
        refreshViews();
        refreshCountryProgress(sizes);
      }

      @Override
      public void onCountryGroupChanged(int oldGroup, int oldPosition, int newGroup, int newPosition)
      {
        refreshViews();
      }

      @Override
      public void onCountryStatusChanged(int group, int position, int oldStatus, int newStatus)
      {
        if (!isAdded())
          return;

        refreshViews();
        final CountryItem countryItem = ActiveCountryTree.getCountryItem(group, position);
        if (newStatus == MapStorage.DOWNLOAD_FAILED)
          UiUtils.checkConnectionAndShowAlert(getActivity(), String.format(getString(R.string.download_country_failed), countryItem.getName()));
        else if (newStatus == MapStorage.DOWNLOADING)
          mDownloadingCountry = countryItem;
        else if (newStatus == MapStorage.ON_DISK && oldStatus == MapStorage.DOWNLOADING)
          exitFragment();
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

    final Location last = LocationHelper.INSTANCE.getLastLocation();
    if (last != null)
      setLatLon(last.getLatitude(), last.getLongitude());

    refreshViews();
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    ActiveCountryTree.removeListener(mCountryListenerId);
  }

  private void setLatLon(double latitude, double longitude)
  {
    mLat = latitude;
    mLon = longitude;
    mCurrentLocationCountryIndex = Framework.nativeGetCountryIndex(mLat, mLon);
  }

  private void refreshCountryName(String name)
  {
    mTvCountry.setText(name);
    mTvActiveCountry.setText(name);
  }

  private void initViews(View view)
  {
    mLlSelectDownload = (LinearLayout) view.findViewById(R.id.ll__select_download);
    mLlActiveDownload = (LinearLayout) view.findViewById(R.id.ll__active_download);
    mLlWithLocation = (LinearLayout) view.findViewById(R.id.ll__location_determined);
    mLlNoLocation = (LinearLayout) view.findViewById(R.id.ll__location_unknown);
    mBtnDownloadMap = (Button) view.findViewById(R.id.btn__download_map);
    mBtnDownloadMap.setOnClickListener(this);
    view.findViewById(R.id.btn__select_map).setOnClickListener(this);
    view.findViewById(R.id.btn__select_other_map).setOnClickListener(this);
    mWpvDownloadProgress = (WheelProgressView) view.findViewById(R.id.wpv__download_progress);
    mWpvDownloadProgress.setCenterDrawable(ContextCompat.getDrawable(getActivity(), R.drawable.ic_close));
    mWpvDownloadProgress.setOnClickListener(this);
    mTvCountry = (TextView) view.findViewById(R.id.tv__country_name);
    mTvActiveCountry = (TextView) view.findViewById(R.id.tv__active_country_name);
    mChbRoutes = (CheckBox) view.findViewById(R.id.chb__routing_too);
    mChbRoutes.setOnCheckedChangeListener(this);
    mChbRoutes.setChecked(true);
  }

  private void refreshViews()
  {
    if (ActiveCountryTree.getTotalDownloadedCount() != 0 || !isAdded())
      return;

    if (ActiveCountryTree.isDownloadingActive())
    {
      mLlSelectDownload.setVisibility(View.GONE);
      mLlActiveDownload.setVisibility(View.VISIBLE);
      if (mDownloadingCountry != null)
        refreshCountryName(mDownloadingCountry.getName());
    }
    else
    {
      mLlSelectDownload.setVisibility(View.VISIBLE);
      mLlActiveDownload.setVisibility(View.GONE);
      if (mLon == 0 || mLat == 0)
      {
        mLlNoLocation.setVisibility(View.VISIBLE);
        mLlWithLocation.setVisibility(View.GONE);
      }
      else
      {
        mLlNoLocation.setVisibility(View.GONE);
        mLlWithLocation.setVisibility(View.VISIBLE);
        refreshCountryName(MapStorage.INSTANCE.countryName(mCurrentLocationCountryIndex));
        refreshCountrySize();
      }
    }
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn__download_map:
      downloadCurrentLocationMap();
      break;
    case R.id.btn__select_map:
    case R.id.btn__select_other_map:
      selectMapForDownload();
      break;
    case R.id.wpv__download_progress:
      cancelCurrentDownload();
      break;
    }
  }

  @Override
  public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
  {
    if (mCurrentLocationCountryIndex == null)
      return;

    refreshCountrySize();
  }

  private void downloadCurrentLocationMap()
  {
    ActiveCountryTree.downloadMapForIndex(mCurrentLocationCountryIndex, storageOptionsRequested());
  }

  private int storageOptionsRequested()
  {
    return mChbRoutes.isChecked() ? StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING : StorageOptions.MAP_OPTION_MAP_ONLY;
  }

  private void selectMapForDownload()
  {
    SearchFragment parent = (SearchFragment) getParentFragment();
    parent.showDownloader();
  }

  private void cancelCurrentDownload()
  {
    ActiveCountryTree.cancelDownloading(mDownloadingGroup, mDownloadingPosition);
  }

  private void refreshCountrySize()
  {
    if (!isAdded())
      return;

    mBtnDownloadMap.setText(getString(R.string.downloader_download_map) +
        " (" + StringUtils.getFileSizeString(MapStorage.INSTANCE.countryRemoteSizeInBytes(mCurrentLocationCountryIndex, storageOptionsRequested())) + ")");
  }

  private void refreshCountryProgress(long[] sizes)
  {
    if (!isAdded())
      return;

    final int percent = (int) (sizes[0] * 100 / sizes[1]);
    mWpvDownloadProgress.setProgress(percent);
  }
}
