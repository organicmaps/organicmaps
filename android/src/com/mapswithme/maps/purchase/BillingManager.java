package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

/**
 * Manages a billing flow for the specific product.
 */
public interface BillingManager<T>
{
  /**
   * Initializes the current billing manager.
   */
  void initialize(@NonNull Activity context);

  /**
   * Destroys the billing manager.
   */
  void destroy();

  /**
   * Launches the billing flow for the product with the specified id.
   * Controls whole native part of billing process.
   *
   * @param productId identifier of the product which is going to be purchased.
   */
  void launchBillingFlowForProduct(@NonNull String productId);

  /**
   * Indicates whether billing is supported for this device or not.
   */
  boolean isBillingSupported();

  /**
   * Queries existing purchases.
   */
  void queryExistingPurchases();

  /**
   * Queries product details for specified products. They will be delivered to the caller
   * through callback {@link T}.
   */
  void queryProductDetails(@NonNull String... productIds);

  /**
   * Adds a billing callback.
   */
  void addCallback(@NonNull T callback);

  /**
   * Removes the billing callback.
   */
  void removeCallback(@NonNull T callback);
}
