package com.mapswithme.maps.purchase;

import android.support.annotation.Nullable;

import com.android.billingclient.api.SkuDetails;

import java.util.List;

public class DefaultSkuDetailsValidationStrategy implements SkuDetailsValidationStrategy
{
  @Override
  public boolean isValid(@Nullable List<SkuDetails> skuDetails)
  {
    return skuDetails != null && !skuDetails.isEmpty();
  }
}
