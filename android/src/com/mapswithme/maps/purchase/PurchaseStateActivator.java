package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface PurchaseStateActivator<State>
{
  void activateState(@NonNull State state);
}
