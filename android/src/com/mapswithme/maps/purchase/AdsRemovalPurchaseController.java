package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.text.TextUtils;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
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
  private final PlayStoreBillingCallback mBillingCallback = new PlaysStoreBillingCallbackImpl();
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

  private static class AdValidationCallbackImpl implements AdsRemovalValidationCallback
  {

    @Override
    public void onValidate(@NonNull AdsRemovalValidationStatus status)
    {
      LOGGER.i(TAG, "Validation static of 'ads removal': " + status);
    }
  }

  private class PlaysStoreBillingCallbackImpl implements PlayStoreBillingCallback
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
        getValidator().validate(purchase.getPurchaseToken());
      }
    }

    @Override
    public void onPurchaseFailure()
    {
      if (getUiCallback() != null)
        getUiCallback().onFailure();
    }

    @Override
    public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
    {
      String token = null;
      String productId = null;
      for (Purchase purchase : purchases)
      {
        if (mProductIds.contains(purchase.getSku()))
        {
          token = purchase.getPurchaseToken();
          productId = purchase.getSku();
          break;
        }
      }

      if (TextUtils.isEmpty(token))
      {
        LOGGER.i(TAG, "Existing purchase token for 'ads removal' not found");
        return;
      }

      LOGGER.i(TAG, "Validating existing purchase token for '" + productId + "'...");
      getValidator().validate(token);
    }
  }
}
