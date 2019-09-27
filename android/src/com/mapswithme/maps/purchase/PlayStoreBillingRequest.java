package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.billingclient.api.BillingClient;

abstract class PlayStoreBillingRequest<T> implements BillingRequest
{
  @NonNull
  private final BillingClient mClient;
  @NonNull
  @BillingClient.SkuType
  private final String mProductType;
  @Nullable
  private final T mCallback;

  PlayStoreBillingRequest(@NonNull BillingClient client, @NonNull String productType,
                          @Nullable T callback)
  {
    mClient = client;
    mProductType = productType;
    mCallback = callback;
  }

  PlayStoreBillingRequest(@NonNull BillingClient client, @NonNull String productType)
  {
    this(client, productType, null);
  }

  @NonNull
  public BillingClient getClient()
  {
    return mClient;
  }

  @NonNull
  @BillingClient.SkuType
  String getProductType()
  {
    return mProductType;
  }

  @Nullable
  public T getCallback()
  {
    return mCallback;
  }
}
