package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.discovery.LocalExpert;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;
import com.mapswithme.util.statistics.GalleryPlacement;
import com.mapswithme.util.statistics.GalleryState;
import com.mapswithme.util.statistics.GalleryType;
import com.mapswithme.util.statistics.Statistics;

import static com.mapswithme.util.statistics.GalleryState.OFFLINE;
import static com.mapswithme.util.statistics.GalleryState.ONLINE;
import static com.mapswithme.util.statistics.GalleryType.LOCAL_EXPERTS;
import static com.mapswithme.util.statistics.GalleryType.VIATOR;

public class Factory
{
  @NonNull
  public static GalleryAdapter createViatorLoadingAdapter
      (@Nullable String cityUrl, @Nullable ItemSelectedListener<Items.Item> listener)
  {
    return new GalleryAdapter<>(new ViatorLoadingAdapterStrategy(cityUrl), listener);
  }

  @NonNull
  public static GalleryAdapter createViatorOfflineAdapter
      (@Nullable ItemSelectedListener<Items.Item> listener, @NonNull GalleryPlacement placement)
  {
    Statistics.INSTANCE.trackGalleryShown(VIATOR, OFFLINE, placement);
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
  public static GalleryAdapter createViatorAdapter(@NonNull ViatorProduct[] products,
                                                   @Nullable String cityUrl,
                                                   @Nullable ItemSelectedListener<Items.ViatorItem>
                                                         listener,
                                                   @NonNull GalleryPlacement placement)
  {
    trackProductGalleryShownOrError(products, VIATOR, ONLINE, placement);
    return new GalleryAdapter<>(new ViatorAdapterStrategy(products, cityUrl), listener);
  }

  @NonNull
  public static GalleryAdapter createSearchBasedAdapter(@NonNull SearchResult[] results,
                                                        @Nullable ItemSelectedListener<Items
                                                            .SearchItem> listener,
                                                        @NonNull GalleryType type,
                                                        @NonNull GalleryPlacement placement)
  {
    trackProductGalleryShownOrError(results, type, OFFLINE, placement);
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
  public static GalleryAdapter createHotelAdapter(@NonNull SearchResult[] results,
                                                  @Nullable ItemSelectedListener<Items
                                                      .SearchItem> listener,
                                                  @NonNull GalleryType type,
                                                  @NonNull GalleryPlacement placement)
  {
    trackProductGalleryShownOrError(results, type, OFFLINE, placement);
    return new GalleryAdapter<>(new HotelAdapterStrategy(results), listener);
  }

  @NonNull
  public static GalleryAdapter createLocalExpertsAdapter(@NonNull LocalExpert[] experts,
                                                         @Nullable String expertsUrl,
                                                         @Nullable ItemSelectedListener<Items
                                                             .LocalExpertItem> listener,
                                                         @NonNull GalleryPlacement placement)
  {
    trackProductGalleryShownOrError(experts, LOCAL_EXPERTS, ONLINE, placement);
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

  private static <Product> void trackProductGalleryShownOrError(@NonNull Product[] products,
                                                                @NonNull GalleryType type,
                                                                @NonNull GalleryState state,
                                                                @NonNull GalleryPlacement placement)
  {
    if (products.length == 0)
      Statistics.INSTANCE.trackGalleryError(type, placement, Statistics.ParamValue.NO_PRODUCTS);
    else
      Statistics.INSTANCE.trackGalleryShown(type, state, placement);
  }
}
