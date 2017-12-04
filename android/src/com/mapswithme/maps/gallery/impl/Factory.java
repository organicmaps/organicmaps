package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.cian.RentPlace;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;

public class Factory
{
  @NonNull
  public static GalleryAdapter createViatorLoadingAdapter(@Nullable String cityUrl,
                                                          @Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new ViatorLoadingAdapterStrategy(cityUrl), listener);
  }

  @NonNull
  public static GalleryAdapter createViatorOfflineAdapter(@Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new ViatorOfflineAdapterStrategy(null), listener);
  }

  @NonNull
  public static GalleryAdapter createViatorErrorAdapter(@Nullable String url,
                                                        @Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new ViatorErrorAdapterStrategy(url), listener);
  }

  @NonNull
  public static GalleryAdapter createCianLoadingAdapter(@Nullable String url,
                                                        @Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new CianLoadingAdapterStrategy(url), listener);
  }

  @NonNull
  public static GalleryAdapter createCianErrorAdapter(@Nullable String url,
                                                      @Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new CianErrorAdapterStrategy(url), listener);
  }

  @NonNull
  public static GalleryAdapter createViatorAdapter(@NonNull ViatorProduct[] products,
                                                   @Nullable String cityUrl,
                                                   @Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new ViatorAdapterStrategy(products, cityUrl), listener);
  }

  @NonNull
  public static GalleryAdapter createCianAdapter(@NonNull RentPlace[] products, @NonNull String url,
                                                 @Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new CianAdapterStrategy(products, url), listener);
  }

  @NonNull
  public static GalleryAdapter createSearchBasedAdapter(@NonNull SearchResult[] results,
                                                        @Nullable GalleryAdapter.ItemSelectedListener listener)
  {
    return new GalleryAdapter<>(new SearchBasedAdapterStrategy(results), listener);
  }
}
