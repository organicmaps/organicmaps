package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;

import java.util.List;

public interface PlayStoreBillingCallback
{
  void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details);
  void onPurchaseSuccessful(@NonNull List<Purchase> purchases);
}
