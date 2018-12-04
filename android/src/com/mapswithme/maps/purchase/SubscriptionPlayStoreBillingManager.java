package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;

import java.util.List;

class SubscriptionPlayStoreBillingManager extends PlayStoreBillingManager
{
  SubscriptionPlayStoreBillingManager(@NonNull String productType)
  {
    super(productType);
  }

  @Override
  public void queryProductDetails(@NonNull List<String> productIds)
  {
    executeBillingRequest(new QueryExistingPurchases(getClientOrThrow(), getProductType(),
                                                     new QueryExistingPurchaseCallback(productIds)));
  }

  private class QueryExistingPurchaseCallback implements PlayStoreBillingCallback
  {
    @NonNull
    private final List<String> mProductIds;

    public QueryExistingPurchaseCallback(@NonNull List<String> productIds)
    {
      mProductIds = productIds;
    }

    @Override
    public void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (getCallback() != null)
        getCallback().onPurchaseDetailsLoaded(details);
    }

    @Override
    public void onPurchaseSuccessful(@NonNull List<Purchase> purchases)
    {
      if (getCallback() != null)
        getCallback().onPurchaseSuccessful(purchases);
    }

    @Override
    public void onPurchaseFailure(int error)
    {
      if (getCallback() != null)
        getCallback().onPurchaseFailure(error);
    }

    @Override
    public void onPurchaseDetailsFailure()
    {
      if (getCallback() != null)
        getCallback().onPurchaseDetailsFailure();
    }

    @Override
    public void onStoreConnectionFailed()
    {
      if (getCallback() != null)
        getCallback().onStoreConnectionFailed();
    }

    @Override
    public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
    {
      if (purchases.isEmpty())
        launchDefaultFlow();
      else
        validateExistingPurchases(purchases);
    }

    private void validateExistingPurchases(@NonNull List<Purchase> purchases)
    {
      if (getCallback() != null)
        getCallback().onPurchasesLoaded(purchases);
    }

    private void launchDefaultFlow()
    {
      executeBillingRequest(new QueryProductDetailsRequest(getClientOrThrow(), getProductType(),
                                                           getCallback(), mProductIds));
    }

    @Override
    public void onConsumptionSuccess()
    {
      if (getCallback() != null)
        getCallback().onConsumptionSuccess();
    }

    @Override
    public void onConsumptionFailure()
    {
      if (getCallback() != null)
        getCallback().onConsumptionFailure();
    }
  }
}
