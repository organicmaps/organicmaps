package com.mapswithme.maps.purchase;

import androidx.annotation.Nullable;

public interface AdsRemovalPurchaseControllerProvider
{
  @Nullable
  PurchaseController<PurchaseCallback> getAdsRemovalPurchaseController();
}
