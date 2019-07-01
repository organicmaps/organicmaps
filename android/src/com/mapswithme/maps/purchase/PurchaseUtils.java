package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.TextUtils;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.List;

public class PurchaseUtils
{
  final static int REQ_CODE_PRODUCT_DETAILS_FAILURE = 1;
  final static int REQ_CODE_PAYMENT_FAILURE = 2;
  final static int REQ_CODE_VALIDATION_SERVER_ERROR = 3;
  final static int REQ_CODE_START_TRANSACTION_FAILURE = 4;
  final static int WEEKS_IN_YEAR = 52;
  final static int MONTHS_IN_YEAR = 12;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = PurchaseUtils.class.getSimpleName();

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

  static boolean hasIncorrectSkuDetails(@NonNull List<SkuDetails> skuDetails)
  {
    for (SkuDetails each : skuDetails)
    {
      if (Period.getInstance(each.getSubscriptionPeriod()) == null)
      {
        String msg = "Unsupported subscription period: '" + each.getSubscriptionPeriod() + "'";
        CrashlyticsUtils.logException(new IllegalStateException(msg));
        LOGGER.e(TAG, msg);
        return true;
      }
    }
    return false;
  }

  static void showPaymentFailureDialog(@NonNull Fragment fragment, @Nullable String tag)
  {
    AlertDialog alertDialog = new AlertDialog.Builder()
        .setReqCode(PurchaseUtils.REQ_CODE_PAYMENT_FAILURE)
        .setTitleId(R.string.bookmarks_convert_error_title)
        .setMessageId(R.string.purchase_error_subtitle)
        .setPositiveBtnId(R.string.back)
        .build();
    alertDialog.show(fragment, tag);
  }

  static void showProductDetailsFailureDialog(@NonNull Fragment fragment, @NonNull String tag)
  {
    AlertDialog alertDialog = new AlertDialog.Builder()
        .setReqCode(PurchaseUtils.REQ_CODE_PRODUCT_DETAILS_FAILURE)
        .setTitleId(R.string.bookmarks_convert_error_title)
        .setMessageId(R.string.discovery_button_other_error_message)
        .setPositiveBtnId(R.string.ok)
        .build();
    alertDialog.show(fragment, tag);
  }

  enum Period
  {
    // Order is important.
    P1Y,
    P1M,
    P1W;

    @Nullable
    static Period getInstance(@Nullable String subscriptionPeriod)
    {
      for (Period each : values())
      {
        if (TextUtils.equals(each.name(), subscriptionPeriod))
          return each;
      }
      return null;
    }
  }
}
