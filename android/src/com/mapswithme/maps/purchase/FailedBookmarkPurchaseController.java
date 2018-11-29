package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.List;

public class FailedBookmarkPurchaseController implements PurchaseController<FailedPurchaseChecker>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = FailedBookmarkPurchaseController.class.getSimpleName();
  @NonNull
  private final PurchaseValidator<ValidationCallback> mValidator;
  @NonNull
  private final BillingManager<PlayStoreBillingCallback> mBillingManager;
  @Nullable
  private FailedPurchaseChecker mCallback;
  @NonNull
  private final ValidationCallback mValidationCallback = new ValidationCallbackImpl();
  @NonNull
  private final PlayStoreBillingCallback mBillingCallback = new PlayStoreBillingCallbackImpl();

  FailedBookmarkPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                                   @NonNull BillingManager<PlayStoreBillingCallback> billingManager)
  {
    mValidator = validator;
    mBillingManager = billingManager;
  }

  @Override
  public void initialize(@NonNull Activity activity)
  {
    mBillingManager.initialize(activity);
    mValidator.addCallback(mValidationCallback);
    mBillingManager.addCallback(mBillingCallback);
  }

  @Override
  public void destroy()
  {
    mBillingManager.destroy();
  }

  @Override
  public boolean isPurchaseSupported()
  {
    throw new UnsupportedOperationException("This purchase controller doesn't respond for " +
                                            "purchase supporting");
  }

  @Override
  public void launchPurchaseFlow(@NonNull String productId)
  {
    throw new UnsupportedOperationException("This purchase controller doesn't support " +
                                            "purchase flow");
  }

  @Override
  public void queryPurchaseDetails()
  {
    throw new UnsupportedOperationException("This purchase controller doesn't support " +
                                            "querying purchase details");
  }

  @Override
  public void validateExistingPurchases()
  {
    mBillingManager.queryExistingPurchases();
  }

  @Override
  public void addCallback(@NonNull FailedPurchaseChecker callback)
  {
    mCallback = callback;
  }

  @Override
  public void removeCallback()
  {
    mCallback = null;
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
        mBillingManager.consumePurchase(PurchaseUtils.parseToken(purchaseData));
        return;
      }

      if (status == ValidationStatus.AUTH_ERROR)
      {
        if (mCallback != null)
          mCallback.onAuthorizationRequired();
        return;
      }

      if (mCallback != null)
        mCallback.onFailedPurchaseDetected(true);
    }
  }

  private class PlayStoreBillingCallbackImpl implements PlayStoreBillingCallback
  {
    @Override
    public void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      // Do nothing by default.
    }

    @Override
    public void onPurchaseSuccessful(@NonNull List<Purchase> purchases)
    {
      // Do nothing by default.
    }

    @Override
    public void onPurchaseFailure(int error)
    {
      // Do nothing by default.
    }

    @Override
    public void onPurchaseDetailsFailure()
    {
      // Do nothing by default.
    }

    @Override
    public void onStoreConnectionFailed()
    {
      // Do nothing by default.
    }

    @Override
    public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
    {
      if (purchases.isEmpty())
      {
        LOGGER.i(TAG, "Non-consumed bookmark purchases not found");
        if (mCallback != null)
          mCallback.onFailedPurchaseDetected(false);
        return;
      }

      if (purchases.size() > 1)
      {
        if (mCallback != null)
          mCallback.onFailedPurchaseDetected(true);
        return;
      }

      Purchase target = purchases.get(0);
      LOGGER.i(TAG, "Validating failed purchase data for '" + target.getSku()
                    + " " + target.getOrderId() + "'...");
      mValidator.validate(null, PrivateVariables.bookmarksVendor(), target.getOriginalJson());
    }

    @Override
    public void onConsumptionSuccess()
    {
      LOGGER.i(TAG, "Failed bookmark purchase consumed and verified");
      if (mCallback != null)
        mCallback.onFailedPurchaseDetected(false);
    }

    @Override
    public void onConsumptionFailure()
    {
      LOGGER.w(TAG, "Failed bookmark purchase not consumed");
      if (mCallback != null)
        mCallback.onFailedPurchaseDetected(true);
    }
  }
}
