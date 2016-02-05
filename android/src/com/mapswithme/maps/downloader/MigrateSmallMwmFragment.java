package com.mapswithme.maps.downloader;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class MigrateSmallMwmFragment extends BaseMwmFragment
                                  implements View.OnClickListener
{
  private WheelProgressView mWheelProgress;
  private View mMigrate;
  private View mMigrating;
  private TextView mTvProgress;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_migrate_small_mwm, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    // TODO listen to map actions callbacks
    initViews(view);
    refreshViews();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    refreshViews();
  }

  private void initViews(View view)
  {
    mMigrate = view.findViewById(R.id.sv__migrate);
    mMigrating = view.findViewById(R.id.ll__downloading);
    Button updateAll = (Button)view.findViewById(R.id.btn__update_all);
    updateAll.setOnClickListener(this);
    UiUtils.updateAccentButton(updateAll);

    view.findViewById(R.id.btn__select_map).setOnClickListener(this);
    view.findViewById(R.id.btn__not_now).setOnClickListener(this);
    mWheelProgress = (WheelProgressView) view.findViewById(R.id.wpv__download_progress);
    mWheelProgress.setCenterDrawable(ContextCompat.getDrawable(getActivity(), R.drawable.ic_close));
    mWheelProgress.setOnClickListener(this);
    mTvProgress = (TextView) view.findViewById(R.id.tv__progress);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn__update_all:
      updateAll();
      break;
    case R.id.btn__select_map:
      selectMaps();
      break;
    case R.id.btn__not_now:
      openDownloader();
      break;
    case R.id.wpv__download_progress:
      cancelUpdate();
      break;
    }
  }

  private void selectMaps()
  {
    getMwmActivity().replaceFragment(SelectMigrationFragment.class, null, null);
  }

  private void updateAll()
  {
    // TODO start map migration
    UiUtils.hide(mMigrate);
    UiUtils.show(mMigrating);
  }

  private void cancelUpdate()
  {
    // TODO cancel migration
    UiUtils.show(mMigrate);
    UiUtils.hide(mMigrating);
  }

  private void openDownloader()
  {
    getMwmActivity().replaceFragment(DownloaderFragment.class, null, null);
  }

  private void refreshViews()
  {
    // TODO show migration UI if maps are migrating
  }

  private void refreshProgress(long[] sizes)
  {
    final int percent = (int) (sizes[0] * 100 / sizes[1]);
    mWheelProgress.setProgress(percent);
    // TODO localize formatting ?
    mTvProgress.setText(StringUtils.getFileSizeString(sizes[0]) + "/" + StringUtils.getFileSizeString(sizes[1]));
  }
}
