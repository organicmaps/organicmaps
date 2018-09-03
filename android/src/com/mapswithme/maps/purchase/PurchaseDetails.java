package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.SkuDetails;

class PurchaseDetails
{
  @NonNull
  private final SkuDetails mDetails;

  PurchaseDetails(@NonNull SkuDetails details)
  {
    mDetails = details;
  }
}
