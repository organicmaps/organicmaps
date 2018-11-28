package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;

import com.mapswithme.maps.bookmarks.data.PaymentData;

public interface BookmarkDownloadCallback
{
  void onAuthorizationRequired();
  void onPaymentRequired(@NonNull PaymentData data);
}
