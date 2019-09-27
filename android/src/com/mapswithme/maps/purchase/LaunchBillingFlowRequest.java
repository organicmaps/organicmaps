package com.mapswithme.maps.purchase;

import android.app.Activity;
import androidx.annotation.NonNull;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingFlowParams;

import static com.mapswithme.maps.purchase.PlayStoreBillingManager.LOGGER;
import static com.mapswithme.maps.purchase.PlayStoreBillingManager.TAG;

class LaunchBillingFlowRequest extends PlayStoreBillingRequest
{
  @NonNull
  private final String mProductId;
  @NonNull
  private final Activity mActivity;

  LaunchBillingFlowRequest(@NonNull Activity activity, @NonNull BillingClient client,
                                  @NonNull String productType, @NonNull String productId)
  {
    super(client, productType);
    mActivity = activity;
    mProductId = productId;
  }

  @Override
  public void execute()
  {
    BillingFlowParams params = BillingFlowParams.newBuilder()
                                                .setSku(mProductId)
                                                .setType(getProductType())
                                                .build();

    int responseCode = getClient().launchBillingFlow(mActivity, params);
    LOGGER.i(TAG, "Launch billing flow response: " + responseCode);
  }
}
