package com.mapswithme.maps.location;

import android.location.Location;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.LocationState;
import com.mapswithme.util.concurrency.UiThread;

class LocationPredictor
{
  private static final String PREDICTOR_PROVIDER = "LocationPredictorProvider";
  private static final long PREDICTION_INTERVAL = 200;
  private static final long MAX_PREDICTION_COUNT = 20;

  private final LocationListener mListener;
  private Location mLastLocation;
  private boolean mGeneratePredictions;
  private int mPredictionCount;

  private final Runnable mRunnable = new Runnable()
  {
    @Override
    public void run()
    {
      if (generatePrediction())
        schedule();
    }
  };

  private void schedule()
  {
    UiThread.runLater(mRunnable, PREDICTION_INTERVAL);
  }

  LocationPredictor(LocationListener listener)
  {
    mListener = listener;
  }

  void resume()
  {
    onMyPositionModeChanged(LocationState.getMode());
  }

  void pause()
  {
    mGeneratePredictions = false;
    mLastLocation = null;
    stop();
  }

  void onMyPositionModeChanged(int mode)
  {
    if (!LocationState.hasLocation(mode))
      mLastLocation = null;

    mGeneratePredictions = (mode == LocationState.FOLLOW_AND_ROTATE);

    stop();
    start();
  }

  void onLocationUpdated(Location location)
  {
    if (location.getProvider().equals(PREDICTOR_PROVIDER))
      return;

    if (location.hasBearing() && location.hasSpeed())
    {
      mLastLocation = new Location(location);
      mLastLocation.setTime(System.currentTimeMillis());
      mLastLocation.setProvider(PREDICTOR_PROVIDER);
      start();
    }
    else
    {
      mLastLocation = null;
      stop();
    }
  }

  private boolean shouldStart()
  {
    return (mLastLocation != null && mGeneratePredictions);
  }

  private void start()
  {
    if (shouldStart())
      schedule();
  }

  private void stop()
  {
    mPredictionCount = 0;
    UiThread.cancelDelayedTasks(mRunnable);
  }

  private boolean generatePrediction()
  {
    if (!shouldStart() || mPredictionCount >= MAX_PREDICTION_COUNT)
      return false;

    mPredictionCount++;

    Location info = new Location(mLastLocation);
    info.setTime(System.currentTimeMillis());

    double elapsed = (info.getTime() - mLastLocation.getTime()) / 1000.0;
    double[] newLatLon = Framework.nativePredictLocation(info.getLatitude(),
                                                         info.getLongitude(),
                                                         info.getAccuracy(),
                                                         info.getBearing(),
                                                         info.getSpeed(),
                                                         elapsed);
    info.setLatitude(newLatLon[0]);
    info.setLongitude(newLatLon[1]);

    mListener.onLocationUpdated(info);
    return true;
  }
}
