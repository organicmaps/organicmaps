package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

/**
 * Provides necessary purchase functionality to the UI. Controls whole platform-specific billing
 * process. This controller has to be used only within {@link #start(Activity)} and {@link #stop()}
 * interval.
 */
public interface PurchaseController
{
  /**
   * Starts the controller.
   * @param activity the activity which controller serves.
   */
  void start(@NonNull Activity activity);

  /**
   * Stops the controller.
   */
  void stop();

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
   * @param id of the product which is going to be purchased.
   */
  void launchPurchaseFlow(@NonNull String productId);
}


