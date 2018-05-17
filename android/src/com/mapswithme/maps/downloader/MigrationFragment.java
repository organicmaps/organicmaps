package com.mapswithme.maps.downloader;

import android.location.Location;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public class MigrationFragment extends BaseMwmFragment
                            implements OnBackPressListener,
                                       MigrationController.Container
{
  private TextView mError;
  private TextView mPrepare;
  private WheelProgressView mProgress;
  private Button mButtonPrimary;
  private Button mButtonSecondary;

  private final View.OnClickListener mButtonClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(final View v)
    {
      MapManager.warnOn3g(getActivity(), null, new Runnable()
      {
        @Override
        public void run()
        {
          boolean keepOld = (v == mButtonPrimary);
          Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MIGRATION_STARTED,
                                         Statistics.params().add(Statistics.EventParam.TYPE, keepOld ? "all_maps"
                                                                                                     : "current_map"));
          MigrationController.get().start(keepOld);
        }
      });
    }
  };

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_migrate, container, false);
  }

  private void checkConnection()
  {
    Utils.checkConnection(getActivity(), R.string.common_check_internet_connection_dialog, new Utils.Proc<Boolean>()
    {
      @Override
      public void invoke(Boolean result)
      {
        if (result)
          return;

        if (getActivity() instanceof MwmActivity)
          ((MwmActivity) getActivity()).closeSidePanel();
        else
          getActivity().finish();
      }
    });
  }

  @Override
  public void onActivityCreated(@Nullable Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);
    checkConnection();
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mError = (TextView) view.findViewById(R.id.error);
    mPrepare = (TextView) view.findViewById(R.id.preparation);
    mProgress = (WheelProgressView) view.findViewById(R.id.wheel_progress);
    mButtonPrimary = (Button) view.findViewById(R.id.button_primary);
    mButtonSecondary = (Button) view.findViewById(R.id.button_secondary);

    mButtonPrimary.setOnClickListener(mButtonClickListener);
    mButtonSecondary.setOnClickListener(mButtonClickListener);

    mProgress.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        MigrationController.get().cancel();
      }
    });

    Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MIGRATION_DIALOG_SEEN);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    MigrationController.get().restore();
  }

  @Override
  public void onStart()
  {
    super.onStart();
    MigrationController.get().attach(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    MigrationController.get().detach();
  }

  @Override
  public void setReadyState()
  {
    UiUtils.show(mButtonPrimary);
    UiUtils.hide(mPrepare, mProgress, mError);

    Location loc = LocationHelper.INSTANCE.getLastKnownLocation();
    UiUtils.showIf(loc != null, mButtonSecondary);
  }

  @Override
  public void setProgressState(String countryName)
  {
    UiUtils.show(mPrepare, mProgress);
    UiUtils.hide(mError, mButtonPrimary, mButtonSecondary);
    mPrepare.setText(String.format("%1$2s %2$s", getString(R.string.downloader_downloading), countryName));
  }

  @Override
  public void setErrorState(int code)
  {
    setReadyState();

    UiUtils.show(mError);

    @StringRes int text;
    switch (code)
    {
    case CountryItem.ERROR_OOM:
      text = R.string.migration_no_space_message;
      break;

    case CountryItem.ERROR_NO_INTERNET:
      text = R.string.common_check_internet_connection_dialog;
      break;

    default:
      text = R.string.country_status_download_failed;
    }

    mError.setText(text);
  }

  @Override
  public void onComplete()
  {
    if (!isAdded())
      return;

    if (getActivity() instanceof MwmActivity)
      ((MwmActivity) getActivity()).showDownloader(false);
    else
      getActivity().recreate();

    Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_MIGRATION_COMPLETE);
  }

  @Override
  public void setProgress(int percents)
  {
    mProgress.setPending(percents == 0);
    if (percents > 0)
      mProgress.setProgress(percents);
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }
}
