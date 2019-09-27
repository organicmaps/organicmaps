package com.mapswithme.maps.purchase;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.base.Savable;

/**
 * Represents a purchase validator. The main of purpose is to validate existing purchase and inform
 * the client code through typed callback {@link T}.<br><br>
 * <b>Important note: </b> one validator can serve only one purchase, i.e. logical link is
 * <b>one-to-one</b>. If you need to validate different purchases you have to create different
 * implementations of this interface.
 */
interface PurchaseValidator<T> extends Savable<Bundle>
{
  /**
   * Validates the purchase with specified purchase data.
   *
   * @param serverId identifier of the purchase on the server.
   * @param vendor vendor of the purchase.
   * @param purchaseData token which describes the validated purchase.
   */
  void validate(@Nullable String serverId, @NonNull String vendor, @NonNull String purchaseData);

  /**
   * Adds validation observer.
   */
  void addCallback(@NonNull T callback);

  /**
   * Removes validation observer.
   */
  void removeCallback();
}
