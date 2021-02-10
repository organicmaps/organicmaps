package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;


public interface PlayStoreBillingCallback
{

  void onProductDetailsFailure();
  void onStoreConnectionFailed();
  void onConsumptionSuccess();
  void onConsumptionFailure();
}
