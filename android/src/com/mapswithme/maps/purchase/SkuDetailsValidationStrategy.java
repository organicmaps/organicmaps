package com.mapswithme.maps.purchase;

import android.support.annotation.Nullable;

import com.android.billingclient.api.SkuDetails;

import java.util.List;

public interface SkuDetailsValidationStrategy
{
  boolean isValid(@Nullable List<SkuDetails> skuDetails);
}
