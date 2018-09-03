package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import java.util.List;

public interface PlayStoreBillingCallback
{
  void onPurchaseDetailsLoaded(@NonNull List<PurchaseDetails> details);
}
