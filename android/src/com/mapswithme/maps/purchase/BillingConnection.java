package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

/**
 * Represents a billing connection abstraction.
 */
public interface BillingConnection
{
  /**
   * Connects to the billing manager.
   */
  void connect();

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
