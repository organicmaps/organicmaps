package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Base64;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PurchaseOperationObservable;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class DefaultPurchaseValidator implements PurchaseValidator<ValidationCallback>,
                                          CoreValidationObserver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = DefaultPurchaseValidator.class.getSimpleName();
  @Nullable
  private ValidationCallback mCallback;
  @NonNull
  private final PurchaseOperationObservable mOperationObservable;

  DefaultPurchaseValidator(@NonNull PurchaseOperationObservable validationObservable)
  {
    mOperationObservable = validationObservable;
  }

  @Override
  public void validate(@Nullable String serverId, @NonNull String vendor,
                       @NonNull String purchaseData)
  {
    String orderId = PurchaseUtils.parseOrderId(purchaseData);
    mOperationObservable.addValidationObserver(orderId, this);
    String encodedPurchaseData = Base64.encodeToString(purchaseData.getBytes(), Base64.DEFAULT);
    Framework.nativeValidatePurchase(serverId == null ? "" : serverId, vendor, encodedPurchaseData);
  }

  @Override
  public void addCallback(@NonNull ValidationCallback callback)
  {
    mCallback = callback;
  }

  @Override
  public void removeCallback()
  {
    mCallback = null;
  }

  @Override
  public void onValidatePurchase(@NonNull ValidationStatus status, @NonNull String serverId,
                                 @NonNull String vendorId, @NonNull String purchaseData)
  {
    LOGGER.i(TAG, "Validation code: " + status);
    String orderId = PurchaseUtils.parseOrderId(purchaseData);
    mOperationObservable.removeValidationObserver(orderId);
    if (mCallback != null)
      mCallback.onValidate(purchaseData, status);
  }
}
