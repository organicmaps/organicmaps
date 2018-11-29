package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Base64;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PurchaseValidationObservable;
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
  private final PurchaseValidationObservable mValidationObservable;

  DefaultPurchaseValidator(@NonNull PurchaseValidationObservable validationObservable)
  {
    mValidationObservable = validationObservable;
  }

  @Override
  public void validate(@Nullable String serverId, @NonNull String vendor,
                       @NonNull String purchaseData)
  {
    String encodedPurchaseData = Base64.encodeToString(purchaseData.getBytes(), Base64.DEFAULT);
    mValidationObservable.addObserver(encodedPurchaseData, this);
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
                                 @NonNull String vendorId, @NonNull String encodedPurchaseData)
  {
    LOGGER.i(TAG, "Validation code: " + status);
    mValidationObservable.removeObserver(encodedPurchaseData, this);
    if (mCallback != null)
    {
      byte[] tokenBytes = Base64.decode(encodedPurchaseData, Base64.DEFAULT);
      mCallback.onValidate(new String(tokenBytes), status);
    }
  }
}
