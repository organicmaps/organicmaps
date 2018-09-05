package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

abstract class AbstractPurchaseController<V, B> implements PurchaseController
{
  @NonNull
  private final PurchaseValidator<V> mValidator;
  @NonNull
  private final BillingManager<B> mBillingManager;

  AbstractPurchaseController(@NonNull PurchaseValidator<V> validator,
                             @NonNull BillingManager<B> billingManager)
  {
    mValidator = validator;
    mBillingManager = billingManager;
  }

  @Override
  public final void start(@NonNull Activity activity)
  {
    mValidator.initialize();
    mBillingManager.initialize(activity);
    onStart(activity);
  }

  @Override
  public final void stop()
  {
    mValidator.destroy();
    mBillingManager.destroy();
    onStop();
  }

  @Override
  public boolean isPurchaseDone()
  {
    return mValidator.hasActivePurchase();
  }

  @Override
  public boolean isPurchaseSupported()
  {
    return mBillingManager.isBillingSupported();
  }

  @Override
  public void launchPurchaseFlow(@NonNull String productId)
  {
    mBillingManager.launchBillingFlowForProduct(productId);
  }

  @NonNull
  PurchaseValidator<V> getValidator()
  {
    return mValidator;
  }

  @NonNull
  BillingManager<B> getBillingManager()
  {
    return mBillingManager;
  }

  abstract void onStart(@NonNull Activity activity);

  abstract void onStop();
}
