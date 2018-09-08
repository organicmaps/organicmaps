package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;

public enum PurchaseFactory
{
  ADS_REMOVAL
      {
        @NonNull
        @Override
        public PurchaseValidator<AdsRemovalValidationCallback> createPurchaseValidator()
        {
          return new AdsRemovalPurchaseValidator();
        }

        @NonNull
        @Override
        BillingManager<PlayStoreBillingCallback> createBillingManager()
        {
          return new PlayStoreBillingManager(BillingClient.SkuType.SUBS);
        }

        @Override
        public PurchaseController createPurchaseController()
        {
          BillingManager<PlayStoreBillingCallback> billingManager = createBillingManager();
          PurchaseValidator<AdsRemovalValidationCallback> validator = createPurchaseValidator();
          return new AdsRemovalPurchaseController(validator, billingManager,
                                                  "ads.removal.monthly.test");
        }
      };

  @NonNull
  abstract PurchaseValidator createPurchaseValidator();

  @NonNull
  abstract BillingManager createBillingManager();

  public abstract PurchaseController createPurchaseController();
}
