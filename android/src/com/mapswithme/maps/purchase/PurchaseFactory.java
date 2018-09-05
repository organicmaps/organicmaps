package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;

public enum PurchaseFactory
{
  ADS_REMOVAL
      {
        @NonNull
        @Override
        public PurchaseValidator<AdValidationCallback> createPurchaseValidator()
        {
          return new AdSubscriptionValidator();
        }

        @NonNull
        @Override
        public BillingManager<PlayStoreBillingCallback> createBillingManager
            (@NonNull Activity activity, @NonNull String... productIds)
        {
          return new PlayStoreBillingManager(BillingClient.SkuType.SUBS, productIds);
        }

        @Override
        public PurchaseController createPurchaseController(@NonNull Activity activity,
                                                           @NonNull String... productIds)
        {
          BillingManager<PlayStoreBillingCallback> billingManager
              = createBillingManager(activity, productIds);
          PurchaseValidator<AdValidationCallback> validator = createPurchaseValidator();
          return new AdPurchaseController(validator, billingManager);
        }
      };

  @NonNull
  public abstract PurchaseValidator createPurchaseValidator();

  @NonNull
  public abstract BillingManager createBillingManager(@NonNull Activity activity,
                                                      @NonNull String... productIds);

  public abstract PurchaseController createPurchaseController(@NonNull Activity activity,
                                                              @NonNull String... productIds);
}
