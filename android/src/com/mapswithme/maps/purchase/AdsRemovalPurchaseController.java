package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

class AdsRemovalPurchaseController extends AbstractPurchaseController<AdsRemovalValidationCallback,
    PlayStoreBillingCallback, AdsRemovalPurchaseCallback>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AdsRemovalPurchaseController.class.getSimpleName();
  @NonNull
  private final AdsRemovalValidationCallback mValidationCallback = new AdValidationCallbackImpl();
  @NonNull
  private final PlayStoreBillingCallback mBillingCallback = new PlayStoreBillingCallbackImpl();
  @NonNull
  private final List<String> mProductIds;

  AdsRemovalPurchaseController(@NonNull PurchaseValidator<AdsRemovalValidationCallback> validator,
                               @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                               @NonNull String... productIds)
  {
    super(validator, billingManager);
    mProductIds = Collections.unmodifiableList(Arrays.asList(productIds));
  }

  @Override
  void onInitialize(@NonNull Activity activity)
  {
    getValidator().addCallback(mValidationCallback);
    getBillingManager().addCallback(mBillingCallback);
    getBillingManager().queryExistingPurchases();
  }

  @Override
  void onDestroy()
  {
    getValidator().removeCallback();
    getBillingManager().removeCallback(mBillingCallback);
  }

  @Override
  public void queryPurchaseDetails()
  {
    getBillingManager().queryProductDetails(mProductIds);
  }

  private class AdValidationCallbackImpl implements AdsRemovalValidationCallback
  {

    @Override
    public void onValidate(@NonNull AdsRemovalValidationStatus status)
    {
      LOGGER.i(TAG, "Validation status of 'ads removal': " + status);
      boolean activateSubscription = status != AdsRemovalValidationStatus.NOT_VERIFIED;
      Framework.nativeSetActiveRemoveAdsSubscription(activateSubscription);

      if (getUiCallback() != null)
        getUiCallback().onValidationStatusObtained(status);
    }
  }

  private class PlayStoreBillingCallbackImpl implements PlayStoreBillingCallback
  {
    @Override
    public void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (getUiCallback() != null)
        getUiCallback().onProductDetailsLoaded(details);
    }

    @Override
    public void onPurchaseSuccessful(@NonNull List<Purchase> purchases)
    {
      for (Purchase purchase : purchases)
      {
        LOGGER.i(TAG, "Validating 'ads removal' purchased '" + purchase.getSku()
                      + "' on backend server...");
        getValidator().validate(purchase.getOriginalJson());
        if (getUiCallback() != null)
          getUiCallback().onValidationStarted();
      }
    }

    @Override
    public void onPurchaseFailure()
    {
      if (getUiCallback() != null)
        getUiCallback().onPaymentFailure();
    }

    @Override
    public void onPurchaseDetailsFailure()
    {
      if (getUiCallback() != null)
        getUiCallback().onProductDetailsFailure();
    }

    @Override
    public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
    {
      String purchaseData = null;
      String productId = null;
      Purchase target = findTargetPurchase(purchases);
      if (target != null)
      {
        purchaseData = target.getOriginalJson();
        productId = target.getSku();
      }

      if (TextUtils.isEmpty(purchaseData))
      {
        LOGGER.i(TAG, "Existing purchase data for 'ads removal' not found");
        return;
      }

      if (!ConnectionState.isWifiConnected())
      {
        LOGGER.i(TAG, "Validation postponed, connection not WI-FI.");
        return;
      }

      LOGGER.i(TAG, "Validating existing purchase data for '" + productId + "'...");
      getValidator().validate(purchaseData);
    }

    @Nullable
    private Purchase findTargetPurchase(@NonNull List<Purchase> purchases)
    {
      for (Purchase purchase: purchases)
      {
        if (mProductIds.contains(purchase.getSku()))
          return purchase;
      }

      return null;
    }
  }
}
