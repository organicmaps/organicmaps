package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;

public class AdRemovalPurchaseValidator implements PurchaseValidator,
                                                   Framework.SubscriptionValidationListener
{
  @Override
  public void initialize()
  {
    Framework.nativeSetSubscriptionValidationListener(this);
  }

  @Override
  public void validate(@NonNull String purchaseToken)
  {
    Framework.nativeValidateSubscription(purchaseToken);
  }

  @Override
  public void hasActivePurchase()
  {
    Framework.nativeHasActiveSubscription();
  }

  @Override
  public void addCallback(@NonNull ValidationCallback callback)
  {
    // Coming soon.
  }

  @Override
  public void removeCallback(@NonNull ValidationCallback callback)
  {
    // Coming soon.
  }

  @Override
  public void onValidateSubscription(@Framework.SubscriptionValidationCode int code)
  {
    // Coming soon.
  }
}
