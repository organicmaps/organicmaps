package com.mapswithme.maps.purchase;

import android.support.annotation.Nullable;

public interface AdsRemovalPurchaseControllerProvider
{
  @Nullable
  PurchaseController<PurchaseCallback> getAdsRemovalPurchaseController();
}
