package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.SkuDetails;

import java.util.List;

public interface PurchaseCallback
{
  void onProductDetailsLoaded(@NonNull List<SkuDetails> details);
  /* all GP */
  void onPaymentFailure(@BillingClient.BillingResponse int error);
  /* all GP */
  void onProductDetailsFailure();
  /* all GP */
  void onStoreConnectionFailed();

  /* single */
  void onValidationStarted();
  void onValidationFinish(boolean success);
}
