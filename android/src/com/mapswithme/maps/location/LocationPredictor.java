package com.mapswithme.maps.location;

import android.location.Location;
import android.os.Handler;
import android.util.Log;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.LocationState;

public class LocationPredictor
{
  final private long PREDICTION_INTERVAL = 200;
  final private long MAX_PREDICTION_COUNT = 20;

  public LocationPredictor(Handler handler, LocationService.LocationListener listener)
  {
    mTimer = handler;
    mListener = listener;
    mPredictionCount = 0;

    mRunnable = new Runnable()
    {
      @Override
      public void run()
      {
        if (generatePrediction())
          mTimer.postDelayed(mRunnable, PREDICTION_INTERVAL);
      }
    };
  }

  public void resume()
  {
    mConnectionSlot = LocationState.INSTANCE.addLocationStateModeListener(this);
    onLocationStateModeChangedCallback(LocationState.INSTANCE.getLocationStateMode());
  }

  public void pause()
  {
    LocationState.INSTANCE.removeLocationStateModeListener(mConnectionSlot);
    mGeneratePredictions = false;
    mLastLocation = null;
    resetTimer();
  }

  public void reset(Location location)
  {
    mLastLocation = location;
    resetTimer();
  }

  private void onLocationStateModeChangedCallback(final int mode)
  {
    mTimer.post(new Runnable()
    {
      @Override
      public void run()
      {
        onLocationStateModeChangedCallbackImpl(mode);
      }
    });
  }

  private void onLocationStateModeChangedCallbackImpl(int mode)
  {
    if (mode < LocationState.NOT_FOLLOW)
      mLastLocation = null;

    mGeneratePredictions = mode == LocationState.ROTATE_AND_FOLLOW;
    resetTimer();
  }

  private void resetTimer()
  {
    mPredictionCount = 0;
    mTimer.removeCallbacks(mRunnable);
    if (mLastLocation != null && mGeneratePredictions)
      mTimer.postDelayed(mRunnable, PREDICTION_INTERVAL);
  }

  private boolean generatePrediction()
  {
    if (mLastLocation == null || mGeneratePredictions == false)
      return false;

    if (mLastLocation.hasBearing() && mLastLocation.hasSpeed() && mPredictionCount < MAX_PREDICTION_COUNT)
    {
      Location info = new Location(mLastLocation);
      info.setTime(System.currentTimeMillis());
      info.setProvider(LocationService.LOCATION_PREDICTOR_PROVIDER);

      long elapsedMillis = info.getTime() - mLastLocation.getTime();

      double[] newLatLon = Framework.predictLocation(info.getLatitude(), info.getLongitude(), info.getBearing(),
                                                     info.getSpeed(), info.getAccuracy(), elapsedMillis / 1000.0);
      info.setLatitude(newLatLon[0]);
      info.setLongitude(newLatLon[1]);

      mListener.onLocationUpdated(info);
    }

    return false;
  }

  private Runnable mRunnable = null;
  private Handler mTimer = null;

  private LocationService.LocationListener mListener = null;
  private Location mLastLocation = null;
  private boolean mGeneratePredictions = false;
  private int mPredictionCount = 0;
  private int mConnectionSlot = 0;
}
