package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class PlayStoreBillingConnection implements BillingConnection,
                                            BillingClientStateListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
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
  public void open()
  {
    LOGGER.i(TAG, "Opening billing connection...");
    mState = State.CONNECTING;
    mBillingClient.startConnection(this);
  }

  @Override
  public void close()
  {
    LOGGER.i(TAG, "Closing billing connection...");
    mBillingClient.endConnection();
    mState = State.CLOSED;
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
    LOGGER.i(TAG, "Connection established to billing client. Response code: " + responseCode);
    if (responseCode == BillingClient.BillingResponse.OK)
    {
      mState = State.CONNECTED;
      if (mListener != null)
        mListener.onConnected();
      return;
    }

    mState = State.DISCONNECTED;
    if (mListener != null)
      mListener.onDisconnected();
  }

  @Override
  public void onBillingServiceDisconnected()
  {
    LOGGER.i(TAG, "Billing client is disconnected.");
    mState = State.DISCONNECTED;
  }

  interface ConnectionListener
  {
    void onConnected();
    void onDisconnected();
  }
}
