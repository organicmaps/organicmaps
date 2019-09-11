package com.mapswithme.maps.promo;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.Html;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.impl.Factory;
import com.mapswithme.maps.gallery.impl.RegularCatalogPromoListener;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.UTM;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.GalleryPlacement;
import com.mapswithme.util.statistics.GalleryType;
import com.mapswithme.util.statistics.Statistics;

import java.util.Objects;

public class CatalogPromoController implements Promo.Listener, Detachable<Activity>
{
  @Nullable
  private Activity mActivity;
  @NonNull
  private RecyclerView mPromoRecycler;
  @NonNull
  private TextView mPromoTitle;
  @NonNull
  private final PlacePageView mPlacePageView;

  public CatalogPromoController(@NonNull PlacePageView placePageView)
  {
    mPlacePageView = placePageView;
    mPromoRecycler = mPlacePageView.findViewById(R.id.catalog_promo_recycler);
    mPromoTitle = mPlacePageView.findViewById(R.id.catalog_promo_title);
    mPromoRecycler.setNestedScrollingEnabled(false);
    LinearLayoutManager layoutManager = new LinearLayoutManager(mPlacePageView.getContext(),
                                                                LinearLayoutManager.HORIZONTAL,
                                                                false);
    mPromoRecycler.setLayoutManager(layoutManager);
    RecyclerView.ItemDecoration decor =
        ItemDecoratorFactory.createPlacePagePromoGalleryDecorator(mPlacePageView.getContext(),
                                                                  LinearLayoutManager.HORIZONTAL);
    mPromoRecycler.addItemDecoration(decor);
  }

  @Override
  public void onCityGalleryReceived(@NonNull PromoCityGallery promo)
  {
    if (mActivity == null)
      throw new AssertionError("Activity cannot be null if promo listener is triggered!");

    Sponsored sponsored = mPlacePageView.getSponsored();
    if (sponsored == null)
      return;

    PromoResponseHandler handler = createPromoResponseHandler(promo);
    if (handler == null)
      return;

    handler.handleResponse(promo);
  }

  @Override
  public void onErrorReceived()
  {
    Statistics.INSTANCE.trackGalleryError(GalleryType.PROMO, GalleryPlacement.PLACEPAGE,
                                          Statistics.ParamValue.NO_PRODUCTS);
  }

  public void updateCatalogPromo(@NonNull NetworkPolicy policy, @Nullable MapObject mapObject)
  {
    if (mActivity == null)
      throw new AssertionError("Activity must be non-null at this point!");

    UiUtils.hide(mPlacePageView, R.id.catalog_promo_container);

    Sponsored sponsored = mPlacePageView.getSponsored();
    if (sponsored == null || mapObject == null)
      return;

    PromoRequester requester = createPromoRequester(sponsored.getType());
    if (requester == null)
      return;

    requester.requestPromo(policy, mapObject);
  }

  @Override
  public void attach(@NonNull Activity object)
  {
    mActivity = object;
    Promo.INSTANCE.setListener(this);
  }

  @Override
  public void detach()
  {
    mActivity = null;
    Promo.INSTANCE.setListener(null);
  }

  @SuppressLint("SwitchIntDef")
  @Nullable
  private static PromoRequester createPromoRequester(@Sponsored.SponsoredType int type)
  {
    switch (type)
    {
      case Sponsored.TYPE_PROMO_CATALOG_POI:
        return new PoiPromoRequester();
      case Sponsored.TYPE_PROMO_CATALOG_CITY:
        return new CityPromoRequester();
      default:
        return null;
    }
  }

  @Nullable
  private PromoResponseHandler createPromoResponseHandler(@NonNull PromoCityGallery promo)
  {
    PromoCityGallery.Item[] items = promo.getItems();
    if (items.length <= 0)
      return null;

    if (items.length == 1)
      return new PoiPromoResponseHandler();

    return new CityPromoResponseHandler();
  }

  interface PromoRequester
  {
    void requestPromo(@NonNull NetworkPolicy policy, @NonNull MapObject mapObject);
  }

  static class PoiPromoRequester implements PromoRequester
  {
    @Override
    public void requestPromo(@NonNull NetworkPolicy policy, @NonNull MapObject mapObject)
    {
      Promo.INSTANCE.nativeRequestPoiGallery(policy, mapObject.getLat(), mapObject.getLon(),
                                             mapObject.getRawTypes(), UTM.UTM_SIGHTSEEINGS_PLACEPAGE_GALLERY);
    }
  }

  static class CityPromoRequester implements PromoRequester
  {
    @Override
    public void requestPromo(@NonNull NetworkPolicy policy, @NonNull MapObject mapObject)
    {
      Promo.INSTANCE.nativeRequestCityGallery(policy, mapObject.getLat(), mapObject.getLon(),
                                              UTM.UTM_LARGE_TOPONYMS_PLACEPAGE_GALLERY);
    }
  }

  interface PromoResponseHandler
  {
    void handleResponse(@NonNull PromoCityGallery promo);
  }

  class PoiPromoResponseHandler implements PromoResponseHandler
  {
    @Override
    public void handleResponse(@NonNull PromoCityGallery promo)
    {
      PromoCityGallery.Item[] items = promo.getItems();
      if (items.length <= 0)
        return;

      UiUtils.show(mPlacePageView, R.id.catalog_promo_container,
                   R.id.promo_poi_description_container, R.id.promo_poi_description_divider,
                   R.id.promo_poi_card);
      UiUtils.hide(mPromoRecycler);
      mPromoTitle.setText(R.string.pp_discovery_place_related_header);

      PromoCityGallery.Item item = items[0];
      PromoCityGallery.Place place = item.getPlace();

      TextView poiName = mPlacePageView.findViewById(R.id.promo_poi_name);
      poiName.setText(place.getName());
      TextView poiDescription = mPlacePageView.findViewById(R.id.promo_poi_description);
      poiDescription.setText(Html.fromHtml(place.getDescription()));

      ImageView poiImage = mPlacePageView.findViewById(R.id.promo_poi_image);
      Glide.with(poiImage.getContext())
           .load(item.getImageUrl())
           .centerCrop()
           .into(poiImage);
      TextView bookmarkName = mPlacePageView.findViewById(R.id.place_single_bookmark_name);
      bookmarkName.setText(item.getName());
      TextView authorName = mPlacePageView.findViewById(R.id.place_single_bookmark_author);
      authorName.setText(item.getAuthor().getName());
    }
  }

  class CityPromoResponseHandler implements PromoResponseHandler
  {
    @Override
    public void handleResponse(@NonNull PromoCityGallery promo)
    {
      UiUtils.show(mPlacePageView, R.id.catalog_promo_container,
                   R.id.catalog_promo_title_divider, R.id.catalog_promo_recycler);
      UiUtils.hide(mPlacePageView, R.id.promo_poi_description_container,
                   R.id.promo_poi_description_divider, R.id.promo_poi_card);
      // TODO: we need to add additional field for title in server protocol (tag).
      mPromoTitle.setText(R.string.guides);
      String url = promo.getMoreUrl();
      RegularCatalogPromoListener promoListener = new RegularCatalogPromoListener(Objects.requireNonNull(mActivity),
                                                                                  GalleryPlacement.PLACEPAGE);
      GalleryAdapter adapter = Factory.createCatalogPromoAdapter(mActivity, promo, url,
                                                                 promoListener,
                                                                 GalleryPlacement.PLACEPAGE);
      mPromoRecycler.setAdapter(adapter);
    }
  }
}
