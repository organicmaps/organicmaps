package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.os.Bundle;

import androidx.annotation.NonNull;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.base.Savable;

/**
 * Provides necessary purchase functionality to the UI. Controls whole platform-specific billing
 * process. This controller has to be used only within
 * {@link com.mapswithme.maps.base.Initializable#initialize}
 * and {@link com.mapswithme.maps.base.Initializable#destroy()}
 * interval.
 */
public interface PurchaseController<T> extends Savable<Bundle>, Initializable<Activity>
{
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
   * Queries product details. They will be delivered to the caller through callback {@link T}.
   */
  void queryProductDetails();

  /**
   * Validates existing purchase. A validation result will be delivered through callback {@link T}.
   */
  void validateExistingPurchases();

  void addCallback(@NonNull T  callback);

  void removeCallback();
}


