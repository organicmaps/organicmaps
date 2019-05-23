package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;

public interface CatalogPromoItem
{
  @NonNull
  String getTitle();

  @NonNull
  String getDescription();

  @NonNull
  String getImgUrl();
}
