package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;
import com.mapswithme.maps.PrivateVariables;

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
    String yearlyProduct = PrivateVariables.adsRemovalYearlyProductId();
    String monthlyProduct = PrivateVariables.adsRemovalMonthlyProductId();
    String weeklyProduct = PrivateVariables.adsRemovalWeeklyProductId();
    return new AdsRemovalPurchaseController(validator, billingManager, yearlyProduct,
                                            monthlyProduct, weeklyProduct);
  }
}
