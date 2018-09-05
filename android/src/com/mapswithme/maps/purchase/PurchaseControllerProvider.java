package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface PurchaseControllerProvider
{
  @NonNull
  public PurchaseController getPurchaseController();
}
