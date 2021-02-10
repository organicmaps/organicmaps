package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Collections;
import java.util.List;


class QueryProductDetailsRequest extends PlayStoreBillingRequest<PlayStoreBillingCallback>
{
  @NonNull
  private final List<String> mProductIds;

  QueryProductDetailsRequest(@NonNull String productType,
                             @Nullable PlayStoreBillingCallback callback,
                             @NonNull List<String> productIds)
  {
    mProductIds = Collections.unmodifiableList(productIds);
  }


}
