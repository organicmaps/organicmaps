package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.text.TextUtils;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;

class AdsRemovalPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, AdsRemovalPurchaseCallback>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AdsRemovalPurchaseController.class.getSimpleName();
  @NonNull
  private final ValidationCallback mValidationCallback = new AdValidationCallbackImpl();
  @NonNull
  private final PlayStoreBillingCallback mBillingCallback = new PlayStoreBillingCallbackImpl();

  AdsRemovalPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                               @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                               @NonNull String... productIds)
  {
    super(validator, billingManager, productIds);
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
    getBillingManager().queryProductDetails(getProductIds());
  }

  private class AdValidationCallbackImpl implements ValidationCallback
  {

    @Override
    public void onValidate(@NonNull ValidationStatus status)
    {
      LOGGER.i(TAG, "Validation status of 'ads removal': " + status);
      if (status == ValidationStatus.VERIFIED)
        Statistics.INSTANCE.trackEvent(Statistics.EventName.INAPP_PURCHASE_VALIDATION_SUCCESS);
      else
        Statistics.INSTANCE.trackPurchaseValidationError(status);

      final boolean activateSubscription = status != ValidationStatus.NOT_VERIFIED;
      final boolean hasActiveSubscription = Framework.nativeHasActiveRemoveAdsSubscription();
      if (!hasActiveSubscription && activateSubscription)
      {
        LOGGER.i(TAG, "Ads removal subscription activated");
        Statistics.INSTANCE.trackPurchaseProductDelivered(PrivateVariables.adsRemovalVendor());
      }
      else if (hasActiveSubscription && !activateSubscription)
      {
        LOGGER.i(TAG, "Ads removal subscription deactivated");
      }

      Framework.nativeSetActiveRemoveAdsSubscription(activateSubscription);

      if (getUiCallback() != null)
        getUiCallback().onValidationFinish(activateSubscription);
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
      Purchase target = findTargetPurchase(purchases);
      if (target == null)
        return;

      LOGGER.i(TAG, "Validating purchase '" + target.getSku() + "' on backend server...");
      getValidator().validate(PrivateVariables.adsRemovalServerId(),
                              PrivateVariables.adsRemovalVendor(), target.getOriginalJson());
      if (getUiCallback() != null)
        getUiCallback().onValidationStarted();
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
        if (Framework.nativeHasActiveRemoveAdsSubscription())
        {
          LOGGER.i(TAG, "Ads removal subscription deactivated");
          Framework.nativeSetActiveRemoveAdsSubscription(false);
        }
        return;
      }

      if (!ConnectionState.isWifiConnected())
      {
        LOGGER.i(TAG, "Validation postponed, connection not WI-FI.");
        return;
      }

      LOGGER.i(TAG, "Validating existing purchase data for '" + productId + "'...");
      getValidator().validate(PrivateVariables.adsRemovalServerId(),
                              PrivateVariables.adsRemovalVendor(), purchaseData);
    }

  }
}
