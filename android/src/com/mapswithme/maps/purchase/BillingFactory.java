package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;

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
        public BillingManager createBillingManager(@NonNull Activity activity,
                                                   @NonNull String... productIds)
        {
          return new PlayStoreBillingManager(activity, BillingClient.SkuType.SUBS, productIds);
        }
      };

  @NonNull
  public abstract PurchaseValidator createPurchaseManager();

  @NonNull
  public abstract BillingManager createBillingManager(@NonNull Activity activity,
                                                      @NonNull String... productIds);
}
