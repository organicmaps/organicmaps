package com.mapswithme.maps.gallery.impl;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.R;
import com.mapswithme.maps.discovery.LocalExpert;
import com.mapswithme.maps.gallery.Constants;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.guides.GuidesGallery;
import com.mapswithme.maps.promo.PromoCityGallery;
import com.mapswithme.maps.promo.PromoEntity;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.widget.placepage.PlacePageView;

import java.util.List;


public class Factory
{
  @NonNull
  public static GalleryAdapter createSearchBasedAdapter(@NonNull SearchResult[] results,
                                                        @Nullable ItemSelectedListener<Items
                                                            .SearchItem> listener,
                                                        @Nullable Items.MoreSearchItem item)
  {
    return new GalleryAdapter<>(new SearchBasedAdapterStrategy(results, item, listener));
  }

  @NonNull
  public static GalleryAdapter createSearchBasedLoadingAdapter(@NonNull Context context)
  {
    return new GalleryAdapter<>(new SimpleLoadingAdapterStrategy(context, null));
  }

  @NonNull
  public static GalleryAdapter createSearchBasedErrorAdapter(@NonNull Context context)
  {
    return new GalleryAdapter<>(new SimpleErrorAdapterStrategy(context, null));
  }

  @NonNull
  public static GalleryAdapter createHotelAdapter(@NonNull Context context,
                                                  @NonNull SearchResult[] results,
                                                  @Nullable ItemSelectedListener<Items
                                                      .SearchItem> listener)
  {
    return new GalleryAdapter<>(new HotelAdapterStrategy(context, results, listener));
  }

  @NonNull
  public static GalleryAdapter createLocalExpertsAdapter(@NonNull LocalExpert[] experts,
                                                         @Nullable String expertsUrl,
                                                         @Nullable ItemSelectedListener<Items
                                                             .LocalExpertItem> listener)
  {
    return new GalleryAdapter<>(new LocalExpertsAdapterStrategy(experts, expertsUrl, listener));
  }

  @NonNull
  public static GalleryAdapter createLocalExpertsErrorAdapter(@NonNull Context context)
  {
    return new GalleryAdapter<>(new LocalExpertsErrorAdapterStrategy(context, null));
  }

  @NonNull
  public static GalleryAdapter createCatalogPromoAdapter(@NonNull Context context,
                                                         @NonNull PromoCityGallery gallery,
                                                         @Nullable String url,
                                                         @Nullable ItemSelectedListener<PromoEntity> listener)
  {
    @SuppressWarnings("ConstantConditions")
    PromoEntity item = new PromoEntity(Constants.TYPE_MORE,
                                       context.getString(R.string.placepage_more_button),
                                       null, url, null, null);
    List<PromoEntity> entities = PlacePageView.toEntities(gallery);
    CatalogPromoAdapterStrategy strategy = new CatalogPromoAdapterStrategy(entities,
                                                                           item,
                                                                           listener);
    return new GalleryAdapter<>(strategy);
  }

  @NonNull
  public static GalleryAdapter createGuidesAdapter(
      @NonNull List<GuidesGallery.Item> items, @Nullable ItemSelectedListener<GuidesGallery.Item> listener)
  {
    GuidesAdapterStrategy strategy = new GuidesAdapterStrategy(items, listener);
    return new GalleryAdapter<>(strategy);
  }

  @NonNull
  public static GalleryAdapter createCatalogPromoLoadingAdapter(@NonNull Context context)
  {
    CatalogPromoLoadingAdapterStrategy strategy = new CatalogPromoLoadingAdapterStrategy(context, null, null);
    return new GalleryAdapter<>(strategy);
  }

  @NonNull
  public static GalleryAdapter createCatalogPromoErrorAdapter(@NonNull Context context,
                                                              @Nullable ItemSelectedListener<Items.Item> listener)
  {
    return new GalleryAdapter<>(new CatalogPromoErrorAdapterStrategy(context, listener));
  }
}
