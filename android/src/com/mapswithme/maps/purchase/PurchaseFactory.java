package com.mapswithme.maps.purchase;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.android.billingclient.api.BillingClient;
import com.mapswithme.maps.PurchaseOperationObservable;

public class PurchaseFactory
{
  private PurchaseFactory()
  {
    // Utility class.
  }

  public static PurchaseController<PurchaseCallback> createAdsRemovalPurchaseController(
      @NonNull Context context)
  {
    return createSubscriptionPurchaseController(context, SubscriptionType.ADS_REMOVAL);
  }

  public static PurchaseController<PurchaseCallback> createBookmarksAllSubscriptionController(
      @NonNull Context context)
  {
    return createSubscriptionPurchaseController(context, SubscriptionType.BOOKMARKS_ALL);
  }

  public static PurchaseController<PurchaseCallback> createBookmarksSightsSubscriptionController(
      @NonNull Context context)
  {
    return createSubscriptionPurchaseController(context, SubscriptionType.BOOKMARKS_SIGHTS);
  }

  @NonNull
  private static PurchaseController<PurchaseCallback> createSubscriptionPurchaseController(
      @NonNull Context context, @NonNull SubscriptionType type)
  {
    BillingManager<PlayStoreBillingCallback> billingManager
        = new PlayStoreBillingManager(BillingClient.SkuType.SUBS);
    PurchaseOperationObservable observable = PurchaseOperationObservable.from(context);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator(observable);
    String[] productIds = type.getProductIds();
    return new SubscriptionPurchaseController(validator, billingManager, type, productIds);
  }

  @NonNull
  static PurchaseController<PurchaseCallback> createBookmarkPurchaseController(
      @NonNull Context context, @Nullable String productId, @Nullable String serverId)
  {
    BillingManager<PlayStoreBillingCallback> billingManager
        = new PlayStoreBillingManager(BillingClient.SkuType.INAPP);
    PurchaseOperationObservable observable = PurchaseOperationObservable.from(context);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator(observable);
    return new BookmarkPurchaseController(validator, billingManager, productId, serverId);
  }

  @NonNull
  public static PurchaseController<FailedPurchaseChecker> createFailedBookmarkPurchaseController(
      @NonNull Context context)
  {
    BillingManager<PlayStoreBillingCallback> billingManager
      = new PlayStoreBillingManager(BillingClient.SkuType.INAPP);
    PurchaseOperationObservable observable = PurchaseOperationObservable.from(context);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator(observable);
    return new FailedBookmarkPurchaseController(validator, billingManager);
  }

  @NonNull
  public static BillingManager<PlayStoreBillingCallback> createInAppBillingManager()
  {
    return new PlayStoreBillingManager(BillingClient.SkuType.INAPP);
  }

  @NonNull
  static BillingManager<PlayStoreBillingCallback> createSubscriptionBillingManager()
  {
    return new PlayStoreBillingManager(BillingClient.SkuType.SUBS);
  }
}
