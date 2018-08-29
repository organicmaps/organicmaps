package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

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
        BillingManager createBillingManager(@NonNull Activity activity)
        {
          return new PlayStoreBillingManager(activity);
        }
      };

  @NonNull
  public abstract PurchaseValidator createPurchaseManager();

  @NonNull
  abstract BillingManager createBillingManager(@NonNull Activity activity);
}
