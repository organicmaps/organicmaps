package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface CoreValidationObserver
{
  void onValidatePurchase(@NonNull ValidationStatus status, @NonNull String serverId,
                          @NonNull String vendorId, @NonNull String purchaseData);
}
