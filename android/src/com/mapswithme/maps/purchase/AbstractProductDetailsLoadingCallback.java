package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;

import java.util.List;

public abstract class AbstractProductDetailsLoadingCallback implements PlayStoreBillingCallback
{
  public abstract void onProductDetailsLoaded(@NonNull List<SkuDetails> details);

  public abstract void onProductDetailsFailure();

  @Override
  public void onPurchaseSuccessful(@NonNull List<Purchase> purchases)
  {
    // Do nothing.
  }

  @Override
  public void onPurchaseFailure(int error)
  {
    // Do nothing.
  }

  @Override
  public void onStoreConnectionFailed()
  {
    // Do nothing.
  }

  @Override
  public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
  {
    // Do nothing.
  }

  @Override
  public void onConsumptionSuccess()
  {
    // Do nothing.
  }

  @Override
  public void onConsumptionFailure()
  {
    // Do nothing.
  }
}
