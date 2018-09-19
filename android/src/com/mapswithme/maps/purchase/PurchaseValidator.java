package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

/**
 * Represents a purchase validator. The main of purpose is to validate existing purchase and inform
 * the client code through typed callback {@link T}.<br><br>
 * <b>Important note: </b> one validator can serve only one purchase, i.e. logical link is
 * <b>one-to-one</b>. If you need to validate different purchases you have to create different
 * implementations of this interface.
 */
interface PurchaseValidator<T>
{
  /**
   * Initializes the validator for further work.
   */
  void initialize();

  /**
   * Destroys this validator.
   */
  void destroy();

  /**
   * Validates the purchase with specified purchase data.
   *
   * @param purchaseData token which describes the validated purchase.
   */
  void validate(@NonNull String purchaseData);

  /**
   * Indicates whether the app has active purchase or not.
   */
  boolean hasActivePurchase();

  /**
   * Adds validation observer.
   */
  void addCallback(@NonNull T callback);

  /**
   * Removes validation observer.
   */
  void removeCallback();
}
