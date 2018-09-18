package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.SkuDetails;

import java.util.List;

public interface AdsRemovalPurchaseCallback
{
  void onProductDetailsLoaded(@NonNull List<SkuDetails> details);
  void onPaymentFailure();
  void onProductDetailsFailure();
}
