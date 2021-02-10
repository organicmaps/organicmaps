package com.mapswithme.maps.purchase;

import android.app.Activity;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class BookmarkPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, PurchaseCallback>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AbstractPurchaseController.class.getSimpleName();

  @NonNull
  private final ValidationCallback mValidationCallback;
  @Nullable
  private final String mServerId;

  BookmarkPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                             @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                             @Nullable String productId, @Nullable String serverId)
  {
    super(validator, billingManager, productId);
    mServerId = serverId;
    mValidationCallback = new ValidationCallbackImpl(mServerId);
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

  private class ValidationCallbackImpl extends AbstractBookmarkValidationCallback
  {

    ValidationCallbackImpl(@Nullable String serverId)
    {
      super(serverId);
    }

    @Override
    void onValidationError(@NonNull ValidationStatus status)
    {
      if (getUiCallback() != null)
        getUiCallback().onValidationFinish(false);
    }

    @Override
    void consumePurchase(@NonNull String purchaseData)
    {
      LOGGER.i(TAG, "Bookmark purchase consuming...");

    }
  }
}
