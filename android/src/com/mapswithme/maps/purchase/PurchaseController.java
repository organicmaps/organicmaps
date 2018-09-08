package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

/**
 * Provides necessary purchase functionality to the UI. Controls whole platform-specific billing
 * process. This controller has to be used only within {@link #initialize(Activity)} and {@link #destroy()}
 * interval.
 */
public interface PurchaseController
{
  /**
   * Initializes the controller.
   * @param activity the activity which controller serves.
   */
  void initialize(@NonNull Activity activity);

  /**
   * Destroys the controller.
   */
  void destroy();

  /**
   * Indicates whether the purchase already done or not.
   */
  boolean isPurchaseDone();

  /**
   * Indicates whether the purchase flow is supported by this device or not.
   */
  boolean isPurchaseSupported();

  /**
   * Launches the purchase flow for the specified product.
   *
   * @param productId id of the product which is going to be purchased.
   */
  void launchPurchaseFlow(@NonNull String productId);
}


