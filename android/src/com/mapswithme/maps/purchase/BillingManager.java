package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

/**
 * Manages a billing flow.
 */
interface BillingManager
{
  /**
   * Launches the billing flow. Controls whole native part of billing.
   * @param productId id of product which is going to be bought by user.
   */
  void launchBillingFlow(@NonNull String productId);

  /**
   * Obtains a purchase token if it exists.
   * @param productId id of product which token is needed for.
   */
  @Nullable
  String obtainTokenForProduct(@NonNull String productId);
}
