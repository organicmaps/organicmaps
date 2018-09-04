package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.UiThread;

/**
 * Manages a billing flow for the specific product.
 */
public interface BillingManager<T>
{
  /**
   * Initializes the current billing manager.
   */
  void initialize();

  /**
   * Destroys the billing manager.
   */
  void destroy();

  /**
   * Launches the billing flow for the product with the specified id.
   * Controls whole native part of billing process.
   *
   * @param productId if of the product which is going to be purchased.
   */
  void launchBillingFlowForProduct(@NonNull String productId);

  /**
   * Indicates whether billing is supported for this device or not.
   */
  boolean isBillingSupported();

  /**
   * Obtains a purchase token if it exists.
   */
  @Nullable
  String obtainPurchaseToken();

  /**
   * Queries product details. They will be delivered to the caller through callback {@link T}.
   */
  void queryProductDetails();

  /**
   * Adds a billing callback.
   */
  void addCallback(@NonNull T callback);

  /**
   * Removes the billing callback.
   */
  void removeCallback(@NonNull T callback);
}
