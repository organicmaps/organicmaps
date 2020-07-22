package com.mapswithme.maps.purchase;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;
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
  private static final String EXTRA_VALIDATED_ORDER_ID = "extra_validated_order_id";
  @Nullable
  private ValidationCallback mCallback;
  @NonNull
  private final PurchaseOperationObservable mOperationObservable;
  @Nullable
  private String mValidatedOrderId;

  DefaultPurchaseValidator(@NonNull PurchaseOperationObservable validationObservable)
  {
    mOperationObservable = validationObservable;
  }

  @Override
  public void validate(@Nullable String serverId, @NonNull String vendor,
                       @NonNull String purchaseData)
  {
    final String orderId = PurchaseUtils.parseOrderId(purchaseData);
    mValidatedOrderId = orderId;
    mOperationObservable.addValidationObserver(orderId, this);
    String encodedPurchaseData = Base64.encodeToString(purchaseData.getBytes(), Base64.DEFAULT);
    Framework.nativeValidatePurchase(serverId == null ? "" : serverId, vendor, encodedPurchaseData);
  }

  @Override
  public void addCallback(@NonNull ValidationCallback callback)
  {
    mCallback = callback;
    if (!TextUtils.isEmpty(mValidatedOrderId))
      mOperationObservable.addValidationObserver(mValidatedOrderId, this);
  }

  @Override
  public void removeCallback()
  {
    mCallback = null;
    if (!TextUtils.isEmpty(mValidatedOrderId))
      mOperationObservable.removeValidationObserver(mValidatedOrderId);
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    outState.putString(EXTRA_VALIDATED_ORDER_ID, mValidatedOrderId);
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mValidatedOrderId = inState.getString(EXTRA_VALIDATED_ORDER_ID);
  }

  @Override
  public void onValidatePurchase(@NonNull ValidationStatus status, @NonNull String serverId,
                                 @NonNull String vendorId, @NonNull String purchaseData,
                                 boolean isTrial)
  {
    LOGGER.i(TAG, "Validation code: " + status);
    String orderId = PurchaseUtils.parseOrderId(purchaseData);
    mOperationObservable.removeValidationObserver(orderId);
    mValidatedOrderId = null;
    if (mCallback != null)
      mCallback.onValidate(purchaseData, status, isTrial);
  }
}
