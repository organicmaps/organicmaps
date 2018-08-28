package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClient.BillingResponse;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.List;

public class PlayStoreBillingManager implements BillingManager, PurchasesUpdatedListener
{
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private final static String TAG = PlayStoreBillingManager.class.getSimpleName();
  @NonNull
  private final Activity mActivity;
  @NonNull
  private final BillingClient mBillingClient;

  PlayStoreBillingManager(@NonNull Activity activity)
  {
    mActivity = activity;
    mBillingClient = BillingClient.newBuilder(mActivity).setListener(this).build();
  }

  @Override
  public void launchBillingFlow(@NonNull String productId)
  {
    @BillingResponse
    int result = mBillingClient.isFeatureSupported(BillingClient.FeatureType.SUBSCRIPTIONS);
    if (result == BillingResponse.FEATURE_NOT_SUPPORTED)
    {
      LOGGER.w(TAG, "Subscription is not supported by this device!");
      return;
    }

    // Coming soon.
  }

  @Nullable
  @Override
  public String obtainTokenForProduct(@NonNull String productId)
  {
    return null;
  }

  @Override
  public void onPurchasesUpdated(int responseCode, @Nullable List<Purchase> purchases)
  {
    // Coming soon.
  }
}
