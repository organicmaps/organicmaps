package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.SkuDetails;

import java.util.List;

public interface PurchaseCallback
{
  void onProductDetailsLoaded(@NonNull List<SkuDetails> details);
  void onPaymentFailure(@BillingClient.BillingResponse int error);
  void onProductDetailsFailure();
  void onStoreConnectionFailed();
  void onValidationStarted();
  void onValidationFinish(boolean success);
}
