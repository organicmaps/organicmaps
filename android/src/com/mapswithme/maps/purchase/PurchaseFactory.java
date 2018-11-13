package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.mapswithme.maps.PrivateVariables;

public class PurchaseFactory
{
  private PurchaseFactory()
  {
    // Utility class.
  }

  @NonNull
  public static PurchaseController<PurchaseCallback> createAdsRemovalPurchaseController()
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
  public static PurchaseController<PurchaseCallback> createBookmarkPurchaseController(
      @Nullable String productId, @Nullable String serverId)
  {
    BillingManager<PlayStoreBillingCallback> billingManager
        = new PlayStoreBillingManager(BillingClient.SkuType.INAPP);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator();
    return new BookmarkPurchaseController(validator, billingManager, productId, serverId);
  }

  @NonNull
  public static PurchaseController<PurchaseCallback> createBookmarkPurchaseController()
  {
    return createBookmarkPurchaseController(null, null);
  }

  @NonNull
  public static PurchaseController<FailedPurchaseChecker> createFailedBookmarkPurchaseController()
  {
    BillingManager<PlayStoreBillingCallback> billingManager
      = new PlayStoreBillingManager(BillingClient.SkuType.INAPP);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator();
    return new FailedBookmarkPurchaseController(validator, billingManager);
  }
}
