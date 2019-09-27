package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

public interface PurchaseStateActivator<State>
{
  void activateState(@NonNull State state);
}
