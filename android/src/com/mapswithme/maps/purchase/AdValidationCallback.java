package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface AdValidationCallback
{
  void onValidate(@NonNull AdValidationStatus status);
}
