package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface AdsRemovalValidationCallback
{
  void onValidate(@NonNull AdsRemovalValidationStatus status);
}
