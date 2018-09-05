package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

class AdPurchaseController extends AbstractPurchaseController<AdValidationCallback, PlayStoreBillingCallback>
{
  @NonNull
  private final AdValidationCallback mValidationCallback;

  AdPurchaseController(@NonNull PurchaseValidator<AdValidationCallback> validator,
                       @NonNull BillingManager<PlayStoreBillingCallback> billingManager)
  {
    super(validator, billingManager);
    mValidationCallback = new StubAdValidationCallback();
  }

  @Override
  void onStart(@NonNull Activity activity)
  {
    getValidator().addCallback(mValidationCallback);
  }

  @Override
  void onStop()
  {
    getValidator().removeCallback(mValidationCallback);
  }

  private static class StubAdValidationCallback implements AdValidationCallback
  {

    @Override
    public void onValidate(@NonNull AdValidationStatus status)
    {
      // Do nothing by default.
    }
  }
}
