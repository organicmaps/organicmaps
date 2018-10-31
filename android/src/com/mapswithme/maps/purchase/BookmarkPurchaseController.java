package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

class BookmarkPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, BookmarkPurchaseCallback>
{
  BookmarkPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                             @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                             @NonNull String... productIds)
  {
    super(validator, billingManager);
    // TODO: coming soon.
  }

  @Override
  void onInitialize(@NonNull Activity activity)
  {
    // TODO: coming soon.
  }

  @Override
  void onDestroy()
  {
    // TODO: coming soon.
  }

  @Override
  public void queryPurchaseDetails()
  {
    // TODO: coming soon.
  }
}
