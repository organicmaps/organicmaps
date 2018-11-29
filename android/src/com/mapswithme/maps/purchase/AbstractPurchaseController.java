package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

abstract class AbstractPurchaseController<V, B, UiCallback extends PurchaseCallback>
    implements PurchaseController<UiCallback>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AbstractPurchaseController.class.getSimpleName();
  @NonNull
  private final PurchaseValidator<V> mValidator;
  @NonNull
  private final BillingManager<B> mBillingManager;
  @Nullable
  private UiCallback mUiCallback;
  @Nullable
  private final List<String> mProductIds;

  AbstractPurchaseController(@NonNull PurchaseValidator<V> validator,
                             @NonNull BillingManager<B> billingManager,
                             @Nullable String... productIds)
  {
    mValidator = validator;
    mBillingManager = billingManager;
    mProductIds = productIds != null ? Collections.unmodifiableList(Arrays.asList(productIds))
                                     : null;
  }

  @Override
  public final void initialize(@NonNull Activity activity)
  {
    mBillingManager.initialize(activity);
    onInitialize(activity);
  }

  @Override
  public final void destroy()
  {
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

  @Override
  public void queryPurchaseDetails()
  {
    if (mProductIds == null)
      throw new IllegalStateException("Product ids must be non-null!");

    getBillingManager().queryProductDetails(mProductIds);
  }

  @Override
  public void validateExistingPurchases()
  {
    getBillingManager().queryExistingPurchases();
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

  @Nullable
  final Purchase findTargetPurchase(@NonNull List<Purchase> purchases)
  {
    if (mProductIds == null)
      return null;

    for (Purchase purchase: purchases)
    {
      if (mProductIds.contains(purchase.getSku()))
        return purchase;
    }

    return null;
  }

  abstract void onInitialize(@NonNull Activity activity);

  abstract void onDestroy();

  abstract class AbstractPlayStoreBillingCallback implements PlayStoreBillingCallback
  {
    @Override
    public void onPurchaseSuccessful(@NonNull List<Purchase> purchases)
    {
      Purchase target = findTargetPurchase(purchases);
      if (target == null)
        return;

      LOGGER.i(TAG, "Validating purchase '" + target.getSku() + " " + target.getOrderId()
                    + "' on backend server...");
      validate(target.getOriginalJson());
      if (getUiCallback() != null)
        getUiCallback().onValidationStarted();
    }

    @Override
    public void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (getUiCallback() != null)
        getUiCallback().onProductDetailsLoaded(details);
    }

    @Override
    public void onPurchaseFailure(@BillingClient.BillingResponse int error)
    {
      if (getUiCallback() != null)
        getUiCallback().onPaymentFailure(error);
    }

    @Override
    public void onPurchaseDetailsFailure()
    {
      if (getUiCallback() != null)
        getUiCallback().onProductDetailsFailure();
    }

    @Override
    public void onStoreConnectionFailed()
    {
      if (getUiCallback() != null)
        getUiCallback().onStoreConnectionFailed();
    }

    @Override
    public void onConsumptionSuccess()
    {
      // Do nothing by default.
    }

    @Override
    public void onConsumptionFailure()
    {
      // Do nothing by default.
    }

    abstract void validate(@NonNull String purchaseData);
  }
}
