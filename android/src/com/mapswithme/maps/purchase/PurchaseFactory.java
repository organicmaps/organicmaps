package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;

public class PurchaseFactory
{
  private PurchaseFactory()
  {
    // Utility class.
  }

  @NonNull
  public static PurchaseController<AdsRemovalPurchaseCallback> createPurchaseController()
  {
    BillingManager<PlayStoreBillingCallback> billingManager
        = new PlayStoreBillingManager(BillingClient.SkuType.SUBS);
    PurchaseValidator<AdsRemovalValidationCallback> validator = new AdsRemovalPurchaseValidator();
    return new AdsRemovalPurchaseController(validator, billingManager,
                                            "ads.removal.monthly.test");
  }
}
