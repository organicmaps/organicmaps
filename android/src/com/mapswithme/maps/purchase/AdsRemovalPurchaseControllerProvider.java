package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface AdsRemovalPurchaseControllerProvider
{
  @NonNull
  PurchaseController<AdsRemovalPurchaseCallback> getAdsRemovalPurchaseController();
}
