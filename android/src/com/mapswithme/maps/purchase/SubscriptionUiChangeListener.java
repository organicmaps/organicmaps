package com.mapswithme.maps.purchase;

public interface SubscriptionUiChangeListener
{
  void onReset();
  void onProductDetailsLoading();
  void onProductDetailsFailure();
  void onPaymentFailure();
  void onPriceSelection();
  void onValidating();
  void onValidationFinish();
  void onPinging();
  void onPingFinish();
  void onCheckNetworkConnection();
}
