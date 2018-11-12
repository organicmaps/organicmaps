package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Base64;

import com.mapswithme.maps.Framework;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class DefaultPurchaseValidator implements PurchaseValidator<ValidationCallback>,
                                          Framework.PurchaseValidationListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = DefaultPurchaseValidator.class.getSimpleName();
  @Nullable
  private ValidationCallback mCallback;

  @Override
  public void initialize()
  {
    Framework.nativeSetPurchaseValidationListener(this);
    LOGGER.i(TAG, "Initializing purchase validator...");
  }

  @Override
  public void destroy()
  {
    Framework.nativeSetPurchaseValidationListener(null);
    LOGGER.i(TAG, "Destroying purchase validator...");
  }

  @Override
  public void validate(@Nullable String serverId, @NonNull String vendor,
                       @NonNull String purchaseData)
  {
    String encodedData = Base64.encodeToString(purchaseData.getBytes(), Base64.DEFAULT);
    Framework.nativeValidatePurchase(serverId, vendor, encodedData);
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
  public void onValidatePurchase(@Framework.PurchaseValidationCode int code,
                                 @NonNull String serverId, @NonNull String vendorId,
                                 @NonNull String purchaseData)
  {
    LOGGER.i(TAG, "Validation code: " + code);
    if (mCallback != null)
    {
      byte[] tokenBytes = Base64.decode(purchaseData, Base64.DEFAULT);
      mCallback.onValidate(new String(tokenBytes), ValidationStatus.values()[code]);
    }
  }
}
