package com.mapswithme.maps.downloader;

import android.location.Location;

import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.statistics.Statistics;

@android.support.annotation.UiThread
final class MigrationController
{
  enum State
  {
    NOT_NECESSARY,
    READY,
    PROGRESS,
    ERROR
  }

  interface Container
  {
    void setReadyState();
    void setProgressState(String countryName);
    void setErrorState(int code);
    void onComplete();

    void setProgress(int percents);
  }

  private static final MigrationController sInstance = new MigrationController();

  private Container mContainer;
  private State mState;
  private String mPrefetchingCountry;
  private int mProgress;
  private int mError;

  private final MapManager.MigrationListener mListener = new MapManager.MigrationListener()
  {
    @Override
    public void onComplete()
    {
      mState = State.NOT_NECESSARY;
      callOnComplete();
    }

    @Override
    public void onProgress(int percent)
    {
      mProgress = percent;
      callUpdateProgress();
    }

    @Override
    public void onError(int code)
    {
      mState = State.ERROR;
      mError = code;
      callStateError();

      MapManager.sendErrorStat(Statistics.EventName.DOWNLOADER_MIGRATION_ERROR, code);
    }
  };

  static MigrationController get()
  {
    return sInstance;
  }

  private MigrationController()
  {
    if (!MapManager.nativeIsLegacyMode())
    {
      mState = State.NOT_NECESSARY;
      return;
    }

    mState = State.READY;
    if (!MapManager.nativeHasSpaceForMigration())
    {
      mState = State.ERROR;
      mError = CountryItem.ERROR_OOM;
    }
  }

  private void callStateReady()
  {
    if (mContainer != null)
      mContainer.setReadyState();
  }

  private void callStateProgress()
  {
    if (mContainer != null)
      mContainer.setProgressState(mPrefetchingCountry);
  }

  private void callStateError()
  {
    if (mContainer != null)
      mContainer.setErrorState(mError);
  }

  private void callUpdateProgress()
  {
    if (mContainer != null)
      mContainer.setProgress(mProgress);
  }

  private void callOnComplete()
  {
    if (mContainer != null)
      mContainer.onComplete();
  }

  void attach(Container container)
  {
    if (mContainer != null)
      throw new IllegalStateException("Must be detached before attach()");

    mContainer = container;
  }

  void detach()
  {
    mContainer = null;
  }

  void restore()
  {
    switch (mState)
    {
    case READY:
      callStateReady();
      break;

    case PROGRESS:
      callStateProgress();
      callUpdateProgress();
      break;

    case ERROR:
      callStateError();
      break;
    }
  }

  void start(boolean keepOld)
  {
    if (mState == State.PROGRESS)
      return;

    Location loc = LocationHelper.INSTANCE.getLastKnownLocation();
    double lat = (loc == null ? 0.0 : loc.getLatitude());
    double lon = (loc == null ? 0.0 : loc.getLongitude());

    mPrefetchingCountry = MapManager.nativeMigrate(mListener, lat, lon, (loc != null), keepOld);
    if (mPrefetchingCountry == null)
      return;

    mState = State.PROGRESS;
    mProgress = 0;
    callStateProgress();
    callUpdateProgress();
  }

  void cancel()
  {
    mState = State.READY;
    callStateReady();

    MapManager.nativeCancelMigration();
  }
}
