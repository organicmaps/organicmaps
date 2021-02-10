package com.mapswithme.maps.purchase;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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

    PurchaseOperationObservable observable = PurchaseOperationObservable.from(context);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator(observable);
    String[] productIds = type.getProductIds();
    return  null;
  }

  @NonNull
  static PurchaseController<PurchaseCallback> createBookmarkPurchaseController(
      @NonNull Context context, @Nullable String productId, @Nullable String serverId)
  {

    PurchaseOperationObservable observable = PurchaseOperationObservable.from(context);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator(observable);
    return null;
  }

  @NonNull
  public static PurchaseController<FailedPurchaseChecker> createFailedBookmarkPurchaseController(
      @NonNull Context context)
  {

    PurchaseOperationObservable observable = PurchaseOperationObservable.from(context);
    PurchaseValidator<ValidationCallback> validator = new DefaultPurchaseValidator(observable);
    return null;
  }

  @NonNull
  public static BillingManager<PlayStoreBillingCallback> createInAppBillingManager()
  {
    return null;
  }

  @NonNull
  static BillingManager<PlayStoreBillingCallback> createSubscriptionBillingManager()
  {
    return null;
  }
}
