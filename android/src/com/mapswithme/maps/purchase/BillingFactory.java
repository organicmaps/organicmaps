package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.SkuDetails;

import java.util.ArrayList;
import java.util.List;

public enum BillingFactory
{
  ADS_REMOVAL
      {
        @NonNull
        @Override
        public PurchaseValidator createPurchaseManager()
        {
          return new AdSubscriptionValidator();
        }

        @NonNull
        @Override
        public BillingManager createBillingManager(@NonNull Activity activity, @NonNull String productId)
        {
          return new PlayStoreBillingManager(activity, productId, BillingClient.SkuType.SUBS);
        }
      };

  @NonNull
  public abstract PurchaseValidator createPurchaseManager();

  @NonNull
  public abstract BillingManager createBillingManager(@NonNull Activity activity, @NonNull String productId);

  @NonNull
  static List<PurchaseDetails> createPurchaseDetailsFrom(@NonNull List<SkuDetails> skuDetails)
  {
    List<PurchaseDetails> details = new ArrayList<>(skuDetails.size());
    for (SkuDetails skuDetail : skuDetails)
      details.add(new PurchaseDetails(skuDetail));
    return details;
  }
}
