package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.Purchase;

import java.util.List;

class QueryExistingPurchases extends PlayStoreBillingRequest<PlayStoreBillingCallback>
{

  QueryExistingPurchases(@NonNull BillingClient client, @NonNull String productType,
                         @Nullable PlayStoreBillingCallback callback)
  {
    super(client, productType, callback);
  }

  @Override
  public void execute()
  {
    Purchase.PurchasesResult purchasesResult = getClient().queryPurchases(getProductType());
    if (purchasesResult == null)
      return;

    List<Purchase> purchases = purchasesResult.getPurchasesList();
    if (purchases == null)
      return;

    if ( getCallback() != null)
      getCallback().onPurchasesLoaded(purchases);
  }
}
