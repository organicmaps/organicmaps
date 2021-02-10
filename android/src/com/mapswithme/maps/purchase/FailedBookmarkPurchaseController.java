package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;


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
  private final ValidationCallback mValidationCallback = new ValidationCallbackImpl(null);


  FailedBookmarkPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                                   @NonNull BillingManager<PlayStoreBillingCallback> billingManager)
  {
    mValidator = validator;
    mBillingManager = billingManager;
  }

  @Override
  public void initialize(@Nullable Activity activity)
  {
    if (activity == null)
      throw new AssertionError("Activity must be non-null!");

    mBillingManager.initialize(activity);
    mValidator.addCallback(mValidationCallback);
  }

  @Override
  public void destroy()
  {
    mBillingManager.destroy();
    mValidator.removeCallback();
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
  public void queryProductDetails()
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

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    mValidator.onSave(outState);
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mValidator.onRestore(inState);
  }

  private class ValidationCallbackImpl extends AbstractBookmarkValidationCallback
  {

    ValidationCallbackImpl(@Nullable String serverId)
    {
      super(serverId);
    }

    @Override
    void onValidationError(@NonNull ValidationStatus status)
    {
      if (status == ValidationStatus.AUTH_ERROR)
      {
        if (mCallback != null)
          mCallback.onAuthorizationRequired();
        return;
      }

      if (mCallback != null)
        mCallback.onFailedPurchaseDetected(true);
    }

    @Override
    void consumePurchase(@NonNull String purchaseData)
    {
      LOGGER.i(TAG, "Failed bookmark purchase consuming...");

    }
  }

  private class PlayStoreBillingCallbackImpl
  {

  }
}
