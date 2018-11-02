package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.Purchase;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

abstract class AbstractPurchaseController<V, B, UiCallback> implements PurchaseController<UiCallback>
{
  @NonNull
  private final PurchaseValidator<V> mValidator;
  @NonNull
  private final BillingManager<B> mBillingManager;
  @Nullable
  private UiCallback mUiCallback;
  @NonNull
  private final List<String> mProductIds;

  AbstractPurchaseController(@NonNull PurchaseValidator<V> validator,
                             @NonNull BillingManager<B> billingManager,
                             @NonNull String... productIds)
  {
    mValidator = validator;
    mBillingManager = billingManager;
    mProductIds = Collections.unmodifiableList(Arrays.asList(productIds));
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

  @SuppressWarnings("AssignmentOrReturnOfFieldWithMutableType")
  @NonNull
  public List<String> getProductIds()
  {
    return mProductIds;
  }

  @NonNull
  final List<Purchase> filterByProductIds(@NonNull List<Purchase> purchases)
  {
    List<Purchase> result = new ArrayList<>();
    for (Purchase purchase: purchases)
    {
      if (getProductIds().contains(purchase.getSku()))
        result.add(purchase);
    }

    return Collections.unmodifiableList(result);
  }

  @Nullable
  final Purchase findTargetPurchase(@NonNull List<Purchase> purchases)
  {
    for (Purchase purchase: purchases)
    {
      if (getProductIds().contains(purchase.getSku()))
        return purchase;
    }

    return null;
  }

  abstract void onInitialize(@NonNull Activity activity);

  abstract void onDestroy();
}
