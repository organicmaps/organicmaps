package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;

class AdSubscriptionValidator implements PurchaseValidator<AdValidationCallback>,
                                                Framework.SubscriptionValidationListener
{
  @Override
  public void initialize()
  {
    Framework.nativeSetSubscriptionValidationListener(this);
  }

  @Override
  public void destroy()
  {
    Framework.nativeSetSubscriptionValidationListener(null);
  }

  @Override
  public void validate(@NonNull String purchaseToken)
  {
    Framework.nativeValidateSubscription(purchaseToken);
  }

  @Override
  public boolean hasActivePurchase()
  {
    return Framework.nativeHasActiveSubscription();
  }

  @Override
  public void addCallback(@NonNull AdValidationCallback callback)
  {
    // Coming soon.
  }

  @Override
  public void removeCallback(@NonNull AdValidationCallback callback)
  {
    // Coming soon.
  }

  @Override
  public void onValidateSubscription(@Framework.SubscriptionValidationCode int code)
  {
    // Coming soon.
  }
}
