package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

public interface ValidationCallback
{
  void onValidate(@NonNull String purchaseData, @NonNull ValidationStatus status, boolean isTrial);
}
