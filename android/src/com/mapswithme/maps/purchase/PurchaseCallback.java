package com.mapswithme.maps.purchase;



public interface PurchaseCallback
{

  void onProductDetailsFailure();
  void onStoreConnectionFailed();
  void onValidationStarted();
  void onValidationFinish(boolean success);
}
