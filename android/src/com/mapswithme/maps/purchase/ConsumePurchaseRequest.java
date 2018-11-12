package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.BillingClient;

import static com.mapswithme.maps.purchase.PlayStoreBillingManager.LOGGER;
import static com.mapswithme.maps.purchase.PlayStoreBillingManager.TAG;

public class ConsumePurchaseRequest extends PlayStoreBillingRequest<PlayStoreBillingCallback>
{
  @NonNull
  private final String mPurchaseToken;

  ConsumePurchaseRequest(@NonNull BillingClient client, @NonNull String productType, @Nullable
      PlayStoreBillingCallback callback, @NonNull String purchaseToken)
  {
    super(client, productType, callback);
    mPurchaseToken = purchaseToken;
  }

  @Override
  public void execute()
  {
    getClient().consumeAsync(mPurchaseToken, this::onConsumeResponseInternal);
  }

  private void onConsumeResponseInternal(@BillingClient.BillingResponse int responseCode,
                                         @NonNull String purchaseToken)
  {
    LOGGER.i(TAG, "Consumption response: " + responseCode);
    if (responseCode == BillingClient.BillingResponse.OK)
    {
      if (getCallback() != null)
        getCallback().onConsumptionSuccess();
      return;
    }

    if (getCallback() != null)
      getCallback().onConsumptionFailure();
  }
}
