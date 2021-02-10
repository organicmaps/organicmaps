package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class PlayStoreBillingConnection implements BillingConnection
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = PlayStoreBillingConnection.class.getSimpleName();

  @NonNull
  private State mState = State.DISCONNECTED;
  @Nullable
  private final ConnectionListener mListener;

  PlayStoreBillingConnection(
                             @Nullable ConnectionListener listener)
  {
    mListener = listener;
  }

  @Override
  public void open()
  {
    LOGGER.i(TAG, "Opening billing connection...");
    mState = State.CONNECTING;
  }

  @Override
  public void close()
  {

    mState = State.CLOSED;
  }

  @NonNull
  @Override
  public State getState()
  {
    return mState;
  }




  interface ConnectionListener
  {
    void onConnected();
    void onDisconnected();
  }
}
