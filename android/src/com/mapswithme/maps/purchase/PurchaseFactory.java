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
  public static PurchaseController<AdsRemovalPurchaseCallback> createAdsRemovalPurchaseController()
  {
    BillingManager<PlayStoreBillingCallback> billingManager
        = new PlayStoreBillingManager(BillingClient.SkuType.SUBS);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator();
    String yearlyProduct = PrivateVariables.adsRemovalYearlyProductId();
    String monthlyProduct = PrivateVariables.adsRemovalMonthlyProductId();
    String weeklyProduct = PrivateVariables.adsRemovalWeeklyProductId();
    return new AdsRemovalPurchaseController(validator, billingManager, yearlyProduct,
                                            monthlyProduct, weeklyProduct);
  }

  @NonNull
  public static PurchaseController<BookmarkPurchaseCallback> createBookmarkPurchaseController(
      @NonNull String productId, @NonNull String serverId)
  {
    BillingManager<PlayStoreBillingCallback> billingManager
        = new PlayStoreBillingManager(BillingClient.SkuType.INAPP);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator();
    return new BookmarkPurchaseController(validator, billingManager, productId, serverId);
  }
}
