package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

public interface CoreStartTransactionObserver
{
  void onStartTransaction(boolean success, @NonNull String serverId, @NonNull String vendorId);
}
