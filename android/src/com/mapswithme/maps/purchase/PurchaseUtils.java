package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.Purchase;
import org.json.JSONException;

class PurchaseUtils
{
  private PurchaseUtils()
  {
    // Utility class.
  }

  @NonNull
  static String parseToken(@NonNull String purchaseData)
  {
    try
    {
      return new Purchase(purchaseData, null).getPurchaseToken();
    }
    catch (JSONException e)
    {
      throw new IllegalArgumentException("Failed to parse purchase token!");
    }
  }
}
