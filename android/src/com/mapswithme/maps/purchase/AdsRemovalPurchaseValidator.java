package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.Framework;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class AdsRemovalPurchaseValidator implements PurchaseValidator<AdsRemovalValidationCallback>,
                                             Framework.SubscriptionValidationListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AdsRemovalPurchaseValidator.class.getSimpleName();
  @Nullable
  private AdsRemovalValidationCallback mCallback;

  @Override
  public void initialize()
  {
    Framework.nativeSetSubscriptionValidationListener(this);
    LOGGER.i(TAG, "Initializing 'ads removal' purchase validator...");
  }

  @Override
  public void destroy()
  {
    Framework.nativeSetSubscriptionValidationListener(null);
    LOGGER.i(TAG, "Destroying 'ads removal' purchase validator...");
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
  public void addCallback(@NonNull AdsRemovalValidationCallback callback)
  {
    mCallback = callback;
  }

  @Override
  public void removeCallback()
  {
    mCallback = null;
  }

  @Override
  public void onValidateSubscription(@Framework.SubscriptionValidationCode int code)
  {
    LOGGER.i(TAG, "Validation code: " + code);
    if (mCallback != null)
      mCallback.onValidate(AdsRemovalValidationStatus.values()[code]);
  }
}
