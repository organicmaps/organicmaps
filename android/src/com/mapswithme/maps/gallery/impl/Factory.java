package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.cian.RentPlace;
import com.mapswithme.maps.discovery.LocalExpert;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;

public class Factory
{
  @NonNull
  public static GalleryAdapter createViatorLoadingAdapter(@Nullable String cityUrl,
                                                          @Nullable ItemSelectedListener<Items.Item>
                                                              listener)
  {
    return new GalleryAdapter<>(new ViatorLoadingAdapterStrategy(cityUrl), listener);
  }

  @NonNull
  public static GalleryAdapter createViatorOfflineAdapter(@Nullable ItemSelectedListener<Items.Item> listener)
  {
    return new GalleryAdapter<>(new ViatorOfflineAdapterStrategy(null), listener);
  }

  @NonNull
  public static GalleryAdapter createViatorErrorAdapter(@Nullable String url,
                                                        @Nullable ItemSelectedListener<Items.Item>
                                                            listener)
  {
    return new GalleryAdapter<>(new ViatorErrorAdapterStrategy(url), listener);
  }

  @NonNull
  public static GalleryAdapter createCianLoadingAdapter(@Nullable String url,
                                                        @Nullable ItemSelectedListener<Items.Item>
                                                            listener)
  {
    return new GalleryAdapter<>(new CianLoadingAdapterStrategy(url), listener);
  }

  @NonNull
  public static GalleryAdapter createCianErrorAdapter(@Nullable String url,
                                                      @Nullable ItemSelectedListener<Items.Item>
                                                          listener)
  {
    return new GalleryAdapter<>(new CianErrorAdapterStrategy(url), listener);
  }

  @NonNull
  public static GalleryAdapter createViatorAdapter(@NonNull ViatorProduct[] products,
                                                   @Nullable String cityUrl,
                                                   @Nullable ItemSelectedListener<Items.ViatorItem>
                                                         listener)
  {
    return new GalleryAdapter<>(new ViatorAdapterStrategy(products, cityUrl), listener);
  }

  @NonNull
  public static GalleryAdapter createCianAdapter(@NonNull RentPlace[] products, @NonNull String url,
                                                 @Nullable ItemSelectedListener<Items.CianItem> listener)
  {
    return new GalleryAdapter<>(new CianAdapterStrategy(products, url), listener);
  }

  @NonNull
  public static GalleryAdapter createSearchBasedAdapter(@NonNull SearchResult[] results,
                                                        @Nullable ItemSelectedListener<Items
                                                            .SearchItem> listener)
  {
    return new GalleryAdapter<>(new SearchBasedAdapterStrategy(results), listener);
  }

  @NonNull
  public static GalleryAdapter createSearchBasedLoadingAdapter()
  {
    return new GalleryAdapter<>(new SimpleLoadingAdapterStrategy(), null);
  }

  @NonNull
  public static GalleryAdapter createSearchBasedErrorAdapter()
  {
    return new GalleryAdapter<>(new SimpleErrorAdapterStrategy(), null);
  }

  @NonNull
  public static GalleryAdapter createLocalExpertsAdapter(@NonNull LocalExpert[] experts,
                                                         @Nullable String expertsUrl,
                                                         @Nullable ItemSelectedListener<Items
                                                             .LocalExpertItem> listener)
  {
    return new GalleryAdapter<>(new LocalExpertsAdapterStrategy(experts, expertsUrl), listener);
  }

  @NonNull
  public static GalleryAdapter createLocalExpertsLoadingAdapter()
  {
    return new GalleryAdapter<>(new LocalExpertsLoadingAdapterStrategy(), null);
  }

  @NonNull
  public static GalleryAdapter createLocalExpertsErrorAdapter()
  {
    return new GalleryAdapter<>(new LocalExpertsErrorAdapterStrategy(), null);
  }
}
