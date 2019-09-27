package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

/**
 * Represents a billing connection abstraction.
 */
public interface BillingConnection
{
  /**
   * Opens a connection to the billing manager.
   */
  void open();

  /**
   * Closes the connection to the billing manager.
   */
  void close();

  /**
   * @return the connection state of the billing manager.
   */
  @NonNull
  State getState();

  enum State
  {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CLOSED
  }
}
