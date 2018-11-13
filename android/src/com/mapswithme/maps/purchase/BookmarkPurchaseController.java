package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import org.json.JSONException;

import java.util.List;

class BookmarkPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, PurchaseCallback>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AbstractPurchaseController.class.getSimpleName();
  @NonNull
  private final PlayStoreBillingCallback mBillingCallback = new PlayStoreBillingCallbackImpl();
  @NonNull
  private final ValidationCallback mValidationCallback = new ValidationCallbackImpl();
  @Nullable
  private final String mServerId;

  BookmarkPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                             @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                             @Nullable String productId, @Nullable String serverId)
  {
    super(validator, billingManager, productId);
    mServerId = serverId;
  }

  @Override
  void onInitialize(@NonNull Activity activity)
  {
    getValidator().addCallback(mValidationCallback);
    getBillingManager().addCallback(mBillingCallback);
  }

  @Override
  void onDestroy()
  {
    getValidator().removeCallback();
    getBillingManager().removeCallback(mBillingCallback);
  }

  @NonNull
  private static String parseToken(@NonNull String purchaseData)
  {
    try
    {
      return new Purchase(purchaseData, null).getPurchaseToken();
    }
    catch (JSONException e)
    {
      throw new IllegalArgumentException("Failed to parse purchase token!");
    }
  }

  private class ValidationCallbackImpl implements ValidationCallback
  {

    @Override
    public void onValidate(@NonNull String purchaseData, @NonNull ValidationStatus status)
    {
      LOGGER.i(TAG, "Validation status of 'paid bookmark': " + status);
      if (status == ValidationStatus.VERIFIED)
      {
        LOGGER.i(TAG, "Bookmark purchase consuming...");
        getBillingManager().consumePurchase(parseToken(purchaseData));
        return;
      }

      if (getUiCallback() != null)
        getUiCallback().onValidationFinish(false);
    }
  }

  private class PlayStoreBillingCallbackImpl extends AbstractPlayStoreBillingCallback
  {
    @Override
    public void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (getUiCallback() != null)
        getUiCallback().onProductDetailsFailure();
    }

    @Override
    void validate(@NonNull String purchaseData)
    {
      getValidator().validate(mServerId, PrivateVariables.bookmarksVendor(), purchaseData);
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
        LOGGER.i(TAG, "Non-consumed bookmark purchases not found");
        return;
      }

      if (!ConnectionState.isWifiConnected())
      {
        LOGGER.i(TAG, "Validation postponed, connection not WI-FI.");
        return;
      }

      LOGGER.i(TAG, "Validating existing purchase data for '" + productId + "'...");
      getValidator().validate(mServerId, PrivateVariables.bookmarksVendor(), purchaseData);
    }

    @Override
    public void onConsumptionSuccess()
    {
      LOGGER.i(TAG, "Bookmark purchase consumed and verified");
      if (getUiCallback() != null)
        getUiCallback().onValidationFinish(true);
    }

    @Override
    public void onConsumptionFailure()
    {
      LOGGER.w(TAG, "Bookmark purchase not consumed");
      if (getUiCallback() != null)
        getUiCallback().onValidationFinish(false);
    }
  }
}
