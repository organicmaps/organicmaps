package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class PlayStoreBillingConnection implements BillingConnection,
                                            BillingClientStateListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = PlayStoreBillingConnection.class.getSimpleName();
  @NonNull
  private final BillingClient mBillingClient;
  @NonNull
  private State mState = State.DISCONNECTED;
  @Nullable
  private final ConnectionListener mListener;

  PlayStoreBillingConnection(@NonNull BillingClient billingClient,
                             @Nullable ConnectionListener listener)
  {
    mBillingClient = billingClient;
    mListener = listener;
  }

  @Override
  public void connect()
  {
    mState = State.CONNECTING;
    mBillingClient.startConnection(this);
  }

  @NonNull
  @Override
  public State getState()
  {
    return mState;
  }

  @Override
  public void onBillingSetupFinished(int responseCode)
  {
    LOGGER.i(TAG, "Setup finished. Response code: " + responseCode);
    if (responseCode == BillingClient.BillingResponse.OK)
    {
      mState = State.CONNECTED;
      if (mListener != null)
        mListener.onConnected();
      return;
    }

    mState = State.DISCONNECTED;
  }

  @Override
  public void onBillingServiceDisconnected()
  {
    mState = State.DISCONNECTED;
  }

  interface ConnectionListener
  {
    void onConnected();
  }
}
