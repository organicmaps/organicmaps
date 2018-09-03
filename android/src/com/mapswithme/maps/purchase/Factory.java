package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;

public enum Factory
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
}
