package com.mapswithme.maps.bookmarks;

import androidx.annotation.NonNull;

import com.mapswithme.maps.bookmarks.data.PaymentData;

public interface BookmarkDownloadCallback
{
  void onAuthorizationRequired();
  void onPaymentRequired(@NonNull PaymentData data);
}
