package com.mapswithme.maps.purchase;

import android.app.Activity;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

class PlayStoreBillingManager
{
  final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  final static String TAG = PlayStoreBillingManager.class.getSimpleName();
  @Nullable
  private Activity mActivity;

  @Nullable
  private PlayStoreBillingCallback mCallback;

  @NonNull
  private BillingConnection mConnection;
  @NonNull
  private final List<BillingRequest> mPendingRequests = new ArrayList<>();



  private void executeBillingRequest(@NonNull BillingRequest request)
  {
    switch (mConnection.getState())
    {
      case CONNECTING:
        mPendingRequests.add(request);
        break;
      case CONNECTED:
        request.execute();
        break;
      case DISCONNECTED:
        mPendingRequests.add(request);
        mConnection.open();
        break;
      case CLOSED:
        throw new IllegalStateException("Billing service connection already closed, " +
                                        "please initialize it again.");
    }
  }

}
