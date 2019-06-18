package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.List;

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
    float price = normalizePrice(skuDetails.getPriceAmountMicros());
    String currencyCode = skuDetails.getPriceCurrencyCode();
    return new ProductDetails(skuDetails.getSku(), price, currencyCode, skuDetails.getTitle());
  }

  @NonNull
  public static String toProductDetailsBundle(@NonNull List<SkuDetails> skuDetails)
  {
    if (skuDetails.isEmpty())
      return "";

    JSONObject bundleJson = new JSONObject();
    for (SkuDetails details: skuDetails)
    {
      JSONObject priceJson = new JSONObject();
      try
      {
        float price = normalizePrice(details.getPriceAmountMicros());
        String currencyCode = details.getPriceCurrencyCode();
        priceJson.put("price_string", Utils.formatCurrencyString(price, currencyCode));
        bundleJson.put(details.getSku(), priceJson);
      }
      catch (Exception e)
      {
        Logger logger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
        String tag = PurchaseUtils.class.getSimpleName();
        String msg = "Failed to form product details bundle for '" + details + "': ";
        logger.e(tag, msg, e);
        CrashlyticsUtils.logException(new RuntimeException(msg, e));
        return "";
      }
    }

    return bundleJson.toString();
  }

  private static float normalizePrice(long priceMicros)
  {
    return priceMicros / 1000000f;
  }
}
