package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import org.json.JSONException;

public class PurchaseUtils
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

  @NonNull
  public static String parseOrderId(@NonNull String purchaseData)
  {
    try
    {
      return new Purchase(purchaseData, null).getOrderId();
    }
    catch (JSONException e)
    {
      throw new IllegalArgumentException("Failed to parse purchase order id!");
    }
  }

  @NonNull
  static ProductDetails toProductDetails(@NonNull SkuDetails skuDetails)
  {
    float price = skuDetails.getPriceAmountMicros() / 1000000;
    String currencyCode = skuDetails.getPriceCurrencyCode();
    return new ProductDetails(skuDetails.getSku(), price, currencyCode, skuDetails.getTitle());
  }
}
