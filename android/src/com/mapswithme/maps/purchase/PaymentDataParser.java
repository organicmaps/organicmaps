package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.bookmarks.data.PaymentData;

public interface PaymentDataParser
{
  @NonNull
  PaymentData parse(@NonNull String url);
  @Nullable
  String getParameterByName(@NonNull String url, @NonNull String name);
}
