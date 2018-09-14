package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

abstract class AbstractPurchaseController<V, B, UiCallback> implements PurchaseController<UiCallback>
{
  @NonNull
  private final PurchaseValidator<V> mValidator;
  @NonNull
  private final BillingManager<B> mBillingManager;
  @Nullable
  private UiCallback mUiCallback;

  AbstractPurchaseController(@NonNull PurchaseValidator<V> validator,
                             @NonNull BillingManager<B> billingManager)
  {
    mValidator = validator;
    mBillingManager = billingManager;
  }

  @Override
  public final void initialize(@NonNull Activity activity)
  {
    mValidator.initialize();
    mBillingManager.initialize(activity);
    onInitialize(activity);
  }

  @Override
  public final void destroy()
  {
    mValidator.destroy();
    mBillingManager.destroy();
    onDestroy();
  }

  @Override
  public void addCallback(@NonNull UiCallback callback)
  {
    mUiCallback = callback;
  }

  @Override
  public void removeCallback()
  {
    mUiCallback = null;
  }

  @Nullable
  UiCallback getUiCallback()
  {
    return mUiCallback;
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

  abstract void onInitialize(@NonNull Activity activity);

  abstract void onDestroy();
}
