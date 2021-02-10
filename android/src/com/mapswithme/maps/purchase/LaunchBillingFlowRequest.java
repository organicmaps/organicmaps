package com.mapswithme.maps.purchase;

import android.app.Activity;
import androidx.annotation.NonNull;



class LaunchBillingFlowRequest
{
  @NonNull
  private final String mProductId;
  @NonNull
  private final Activity mActivity;

  LaunchBillingFlowRequest(@NonNull Activity activity,
                                  @NonNull String productType, @NonNull String productId)
  {

    mActivity = activity;
    mProductId = productId;
  }


}
