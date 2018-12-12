package com.mapswithme.maps.purchase;

import android.support.annotation.Nullable;

import com.android.billingclient.api.SkuDetails;
import com.mapswithme.util.CrashlyticsUtils;

import java.util.List;

import static com.mapswithme.maps.purchase.PlayStoreBillingManager.LOGGER;
import static com.mapswithme.maps.purchase.PlayStoreBillingManager.TAG;

public class SubscriptionSkuDetailsValidationStrategy extends DefaultSkuDetailsValidationStrategy
{
  @Override
  public boolean isValid(@Nullable List<SkuDetails> skuDetails)
  {
    boolean hasDetails = super.isValid(skuDetails);
    return hasDetails && !hasIncorrectSkuDetails(skuDetails) ;
  }

  private static boolean hasIncorrectSkuDetails(@Nullable List<SkuDetails> skuDetails)
  {
    if (skuDetails == null)
      return true;

    for (SkuDetails each : skuDetails)
    {
      if (AdsRemovalPurchaseDialog.Period.getInstance(each.getSubscriptionPeriod()) == null)
      {
        String msg = "Unsupported subscription period: '" + each.getSubscriptionPeriod() + "'";
        CrashlyticsUtils.logException(new IllegalStateException(msg));
        LOGGER.e(TAG, msg);
        return true;
      }
    }
    return false;
  }
}
