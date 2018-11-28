package com.mapswithme.maps.purchase;

public interface FailedPurchaseChecker
{
  void onFailedPurchaseDetected(boolean isDetected);
  void onAuthorizationRequired();
}
