package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

/**
 * Provides necessary purchase functionality to the UI. Controls whole platform-specific billing
 * process. This controller has to be used only within {@link #initialize(Activity)} and {@link #destroy()}
 * interval.
 */
public interface PurchaseController<T>
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
   * Indicates whether the purchase flow is supported by this device or not.
   */
  boolean isPurchaseSupported();

  /**
   * Launches the purchase flow for the specified product. The purchase results will be delivered
   * through {@link T} callback.
   *
   * @param productId id of the product which is going to be purchased.
   */
  void launchPurchaseFlow(@NonNull String productId);

  /**
   * Queries purchase details. They will be delivered to the caller through callback {@link T}.
   */
  void queryPurchaseDetails();

  /**
   * Validates existing purchase. A validation result will be delivered through callback {@link T}.
   */
  void validateExistingPurchases();

  void addCallback(@NonNull T  callback);

  void removeCallback();
}


