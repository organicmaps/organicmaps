package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;


class SubscriptionPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, PurchaseCallback>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = SubscriptionPurchaseController.class.getSimpleName();
  @NonNull
  private final ValidationCallback mValidationCallback = new ValidationCallbackImpl();

  @NonNull
  private final SubscriptionType mType;

  SubscriptionPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                                 @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                                 @NonNull SubscriptionType subscriptionType,
                                 @NonNull String... productIds)
  {
    super(validator, billingManager, productIds);
    mType = subscriptionType;
  }

  @Override
  void onInitialize(@NonNull Activity activity)
  {
    getValidator().addCallback(mValidationCallback);
  }

  @Override
  void onDestroy()
  {
    getValidator().removeCallback();
  }

  private class ValidationCallbackImpl implements ValidationCallback
  {
    @Override
    public void onValidate(@NonNull String purchaseData, @NonNull ValidationStatus status,
                           boolean isTrial)
    {
      LOGGER.i(TAG, "Validation status of '" + mType + "': " + status);
      if (status == ValidationStatus.VERIFIED)
        Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName
                                                   .INAPP_PURCHASE_VALIDATION_SUCCESS,
                                               mType.getServerId());
      else
        Statistics.INSTANCE.trackPurchaseValidationError(mType.getServerId(), status);

      final boolean shouldActivateSubscription = status != ValidationStatus.NOT_VERIFIED;
      final boolean hasActiveSubscription = Framework.nativeHasActiveSubscription(mType.ordinal());
      if (!hasActiveSubscription && shouldActivateSubscription)
      {
        LOGGER.i(TAG, "'" + mType + "' subscription activated");
        Statistics.INSTANCE.trackPurchaseProductDelivered(mType.getServerId(), mType.getVendor(),
                                                          isTrial);
      }
      else if (hasActiveSubscription && !shouldActivateSubscription)
      {
        LOGGER.i(TAG, "'" + mType + "' subscription deactivated");
      }

      Framework.nativeSetActiveSubscription(mType.ordinal(), shouldActivateSubscription, isTrial);

      if (getUiCallback() != null)
        getUiCallback().onValidationFinish(shouldActivateSubscription);
    }
  }

  private class PlayStoreBillingCallbackImpl extends AbstractPlayStoreBillingCallback
  {
    @Override
    void validate(@NonNull String purchaseData)
    {
      getValidator().validate(mType.getServerId(), mType.getVendor(), purchaseData);
    }


  }
}
