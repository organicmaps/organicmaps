package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;

import com.mapswithme.maps.bookmarks.data.PaymentData;

interface PaymentDataParser
{
  @NonNull
  PaymentData parse(@NonNull String url);
}
