package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

/**
 * Represents a purchase validator. The main of purpose is to validate existing purchase and inform
 * the client code through typed callback {@link T}.<br><br>
 * <b>Important note: </b> one validator can serve only one purchase, i.e. logical link is
 * <b>one-to-one</b>. If you need to validate different purchases you have to create different
 * implementations of this interface.
 */
public interface PurchaseValidator<T>
{
  /**
   * Initializes validator for further work.
   */
  void initialize();

  /**
   * Validates the purchase with specified token.
   *
   * @param purchaseToken token which describes the validated purchase.
   */
  void validate(@NonNull String purchaseToken);

  /**
   * Indicates whether the app has active purchase or not.
   */
  void hasActivePurchase();

  /**
   * Ads observer of validation.
   */
  void addCallback(@NonNull T callback);

  /**
   * Removes observer of validation.
   */
  void removeCallback(@NonNull T callback);
}
