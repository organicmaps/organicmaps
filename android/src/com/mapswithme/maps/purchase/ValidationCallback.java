package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface ValidationCallback
{
  void onValidate(@NonNull String purchaseData, @NonNull ValidationStatus status);
}
