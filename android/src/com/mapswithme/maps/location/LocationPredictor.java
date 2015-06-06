package com.mapswithme.maps.location;

import android.location.Location;
import android.os.Handler;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.LocationState;

public class LocationPredictor
{
  final private long PREDICTION_INTERVAL = 200;
  final private long MAX_PREDICTION_COUNT = 20;

  private Runnable mRunnable;
  private Handler mHandler;

  private LocationHelper.LocationListener mListener;
  private Location mLastLocation;
  private boolean mGeneratePredictions;
  private int mPredictionCount;
  private int mConnectionSlot = LocationState.SLOT_UNDEFINED;

  public LocationPredictor(Handler handler, LocationHelper.LocationListener listener)
  {
    mHandler = handler;
    mListener = listener;

    mRunnable = new Runnable()
    {
      @Override
      public void run()
      {
        if (generatePrediction())
          mHandler.postDelayed(mRunnable, PREDICTION_INTERVAL);
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
    resetHandler();
  }

  public void reset(Location location)
  {
    if (location.hasBearing() && location.hasSpeed())
    {
      mLastLocation = new Location(location);
      mLastLocation.setTime(System.currentTimeMillis());
      mLastLocation.setProvider(LocationHelper.LOCATION_PREDICTOR_PROVIDER);
    }
    else
      mLastLocation = null;

    resetHandler();
  }

  private void onLocationStateModeChangedCallback(final int mode)
  {
    mHandler.post(new Runnable()
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

    mGeneratePredictions = (mode == LocationState.ROTATE_AND_FOLLOW);
    resetHandler();
  }

  private boolean isPredict()
  {
    return mLastLocation != null && mGeneratePredictions;
  }

  private void resetHandler()
  {
    mPredictionCount = 0;

    mHandler.removeCallbacks(mRunnable);

    if (isPredict())
      mHandler.postDelayed(mRunnable, PREDICTION_INTERVAL);
  }

  private boolean generatePrediction()
  {
    if (!isPredict())
      return false;

    if (mPredictionCount < MAX_PREDICTION_COUNT)
    {
      ++mPredictionCount;

      Location info = new Location(mLastLocation);
      info.setTime(System.currentTimeMillis());

      final long elapsedMillis = info.getTime() - mLastLocation.getTime();

      double[] newLatLon = Framework.predictLocation(info.getLatitude(), info.getLongitude(),
              info.getAccuracy(), info.getBearing(),
              info.getSpeed(), elapsedMillis / 1000.0);
      info.setLatitude(newLatLon[0]);
      info.setLongitude(newLatLon[1]);

      mListener.onLocationUpdated(info);
      return true;
    }

    return false;
  }
}
