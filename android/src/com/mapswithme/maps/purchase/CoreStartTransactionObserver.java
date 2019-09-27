package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

public interface CoreStartTransactionObserver
{
  void onStartTransaction(boolean success, @NonNull String serverId, @NonNull String vendorId);
}
